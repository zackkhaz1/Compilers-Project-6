#include <assert.h>

#include "name_analysis.hpp"
#include "type_analysis.hpp"

namespace crona {

TypeAnalysis * TypeAnalysis::build(NameAnalysis * nameAnalysis){
	TypeAnalysis * typeAnalysis = new TypeAnalysis();
	auto ast = nameAnalysis->ast;	
	typeAnalysis->ast = ast;

	ast->typeAnalysis(typeAnalysis);
	if (typeAnalysis->hasError){
		return nullptr;
	}

	return typeAnalysis;

}

void ProgramNode::typeAnalysis(TypeAnalysis * typing){
	for (auto decl : *myGlobals){
		decl->typeAnalysis(typing);
	}
	typing->nodeType(this, BasicType::VOID());
}

void IDNode::typeAnalysis(TypeAnalysis * typing){
	typing->nodeType(this, mySymbol->getDataType());
}

void VarDeclNode::typeAnalysis(TypeAnalysis * typing){
	myType->typeAnalysis(typing);
	const DataType * declaredType = typing->nodeType(myType);
	//We assume that the type that comes back is valid,
	// otherwise we wouldn't have passed nameAnalysis
	typing->nodeType(this, declaredType);
}

void FnDeclNode::typeAnalysis(TypeAnalysis * typing){
	myRetType->typeAnalysis(typing);
	const DataType * retDataType = typing->nodeType(myRetType);

	std::list<const DataType *> * formalTypes = 
		new std::list<const DataType *>();
	for (auto formal : *myFormals){
		formal->typeAnalysis(typing);
		formalTypes->push_back(typing->nodeType(formal));
	}	

	
	typing->nodeType(this, new FnType(formalTypes, retDataType));

	typing->setCurrentFnType(typing->nodeType(this)->asFn());
	for (auto stmt : *myBody){
		stmt->typeAnalysis(typing);
	}
	typing->setCurrentFnType(nullptr);
}

static const DataType * typeAssignOpd(TypeAnalysis * typing, ExpNode * opd){
	opd->typeAnalysis(typing);
	const DataType * type = typing->nodeType(opd);

	//Errors are invalid, but don't cause re-reports
	if (type->asError()){ return nullptr; }

	//Valid types are returned
	if (type->asBasic()){ 
		if (type->isVoid()){
			typing->errAssignOpd(opd->line(),opd->col());
			return nullptr;
		}
		return type; 
	}

	if (type->isArray()){
		return type;
	}

	//Valid types are returned
	/*
	if (const PtrType * asPtr = type->asPtr()){ 
		return type; 
	}
	*/

	//Invalid types are reported and skip operator check
	typing->errAssignOpd(opd->line(),opd->col());
	return nullptr;
}

void AssignExpNode::typeAnalysis(TypeAnalysis * typing){
	const DataType * dstType = typeAssignOpd(typing, myDst);
	const DataType * srcType = typeAssignOpd(typing, mySrc);

	if (!dstType || !srcType){
		typing->nodeType(this, ErrorType::produce());
		return;
	}

	if (dstType == srcType){
		typing->nodeType(this, dstType);
		return;
	}

	const ArrayType * dstArray = dstType->asArray();
	const ArrayType * srcArray = srcType->asArray();
	if (dstArray && srcArray){
		const BasicType * dstBase = dstArray->baseType();
		const BasicType * srcBase = srcArray->baseType();
		if (dstBase == srcBase){
			typing->nodeType(this, dstType);
			return;
		}
	}

	if (srcType->isByte() && dstType->isInt()){
		//Promote
		mySrc = new ByteToIntNode(mySrc);
		typing->nodeType(mySrc, BasicType::INT());
		typing->nodeType(this, BasicType::INT());
		return;
	}
	/*
	if (dstType->asPtr()){
		if (const PtrType * srcPtr = srcType->asPtr()){
			if (srcPtr->decLevel() == BasicType::VOID()){
				typing->nodeType(this, dstType);
				return;
			}
		}
	}
	*/

	typing->errAssignOpr(line(), col());
	typing->nodeType(this, ErrorType::produce());
	return;
}

void CallExpNode::typeAnalysis(TypeAnalysis * typing){

	std::list<const DataType *> * aList = new std::list<const DataType *>();
	for (auto actual : *myArgs){
		actual->typeAnalysis(typing);
		aList->push_back(typing->nodeType(actual));
	}

	SemSymbol * calleeSym = myID->getSymbol();
	assert(calleeSym != nullptr);
	const DataType * calleeType = calleeSym->getDataType();
	const FnType * fnType = calleeType->asFn();
	if (fnType == nullptr){
		typing->errCallee(myID->line(), myID->col());
		typing->nodeType(this, ErrorType::produce());
		return;
	}

	const std::list<const DataType *>* fList = fnType->getFormalTypes();
	if (aList->size() != fList->size()){
		typing->errArgCount(line(), col());
		//Note: we still consider the call to return the 
		// return type
	} else {
		auto actualTypesItr = aList->begin();
		auto formalTypesItr = fList->begin();
		auto actualsItr = myArgs->begin();
		while(actualTypesItr != aList->end()){
			const DataType * actualType = *actualTypesItr;
			const DataType * formalType = *formalTypesItr;
			ExpNode * actual = *actualsItr;
			auto actualsItrOld = actualsItr;
			actualTypesItr++;
			formalTypesItr++;
			actualsItr++;

			//Matching to error is ignored
			if (actualType->asError()){ continue; }
			if (formalType->asError()){ continue; }

			//Ok match
			if (formalType == actualType){ continue; }

			const ArrayType * formalArr = formalType->asArray();
			const ArrayType * actualArr = actualType->asArray();
			if (formalArr && actualArr){
				if (formalArr->baseType() == actualArr->baseType()){
					continue;
				}
			}

			//Promote
			if (formalType->isInt() && actualType->isByte()){
				//Promote
				ByteToIntNode * up = new ByteToIntNode(actual);
				typing->nodeType(up, BasicType::INT());
				
				actualsItrOld = myArgs->erase(actualsItrOld);
				actualsItrOld = myArgs->insert(actualsItrOld, up);
				
				continue;
			}

			//Bad match
			typing->errArgMatch(actual->line(), actual->col());
			typing->nodeType(this, ErrorType::produce());
		}
	}

	typing->nodeType(this, fnType->getReturnType());
	return;
}

void ByteToIntNode::typeAnalysis(TypeAnalysis * typing){
	//We never really expect this to happen, but we can handle it
	typing->nodeType(this, BasicType::INT());
}

void NegNode::typeAnalysis(TypeAnalysis * typing){
	myExp->typeAnalysis(typing);
	const DataType * subType = typing->nodeType(myExp);

	//Propagate error, don't re-report
	if (subType->asError()){
		typing->nodeType(this, subType);
		return;
	} else if (subType->isInt()){
		typing->nodeType(this, BasicType::INT());
	}  else if (subType->isByte()){
		ByteToIntNode * promote = new ByteToIntNode(myExp);
		myExp = promote;
		typing->nodeType(promote, BasicType::INT());
		typing->nodeType(this, BasicType::INT());
	} else {
		typing->errMathOpd(myExp->line(), myExp->col());
		typing->nodeType(this, ErrorType::produce());
	}
}

void NotNode::typeAnalysis(TypeAnalysis * typing){
	myExp->typeAnalysis(typing);
	const DataType * childType = typing->nodeType(myExp);

	if (childType->asError() != nullptr){
		typing->nodeType(this, ErrorType::produce());
		return;
	}

	if (childType->isBool()){
		typing->nodeType(this, childType);
		return;
	} else {
		typing->errLogicOpd(myExp->line(), myExp->col());
		typing->nodeType(this, ErrorType::produce());
		return;
	}
}

void TypeNode::typeAnalysis(TypeAnalysis * typing){
	typing->nodeType(this, this->getType());
}

static bool typeMathOpd(TypeAnalysis * typing, ExpNode * opd){
	opd->typeAnalysis(typing);
	const DataType * type = typing->nodeType(opd);
	if (type->isInt()){ return true; }
	if (type->isByte()){ return true; }
	if (type->asError()){
		//Don't re-report an error, but don't check for
		// incompatibility
		return false;
	}

	typing->errMathOpd(opd->line(), opd->col());
	return false;
}

static const DataType * getEltType(const ArrayType * arrType){
	if (arrType == nullptr){
		return ErrorType::produce();
	}
	return arrType->baseType();
}

void IndexNode::typeAnalysis(TypeAnalysis * typing){
	myBase->typeAnalysis(typing);
	myOffset->typeAnalysis(typing);

	const DataType * baseType = typing->nodeType(myBase);
	const DataType * offType = typing->nodeType(myOffset);

	if (offType->asError() || baseType->asError()){
		typing->nodeType(this, ErrorType::produce());
		return;
	}

	const ArrayType * asArr = baseType->asArray();
	const DataType * eltType = getEltType(asArr);
	if (asArr == nullptr){
		typing->errArrayID(myBase->line(), myBase->col());
	}
	if (offType->isInt()){
		typing->nodeType(this, eltType);
		return;
	}
	if (offType->isByte()){
		typing->nodeType(this, eltType);
		return;
	}

	typing->nodeType(this, ErrorType::produce());
	typing->errArrayIndex(myOffset->line(), myOffset->col());

	/*
	if (const PtrType * asPtr = baseType->asPtr()){
		typing->nodeType(this, asPtr->decLevel());
	} else {
		typing->badPtrBase(myBase->line(), myBase->col());
	}
	*/
}

void BinaryExpNode::binaryMathTyping(
	TypeAnalysis * typing
){
	bool lhsValid = typeMathOpd(typing, myExp1);
	bool rhsValid = typeMathOpd(typing, myExp2);
	if (!lhsValid || !rhsValid){
		typing->nodeType(this, ErrorType::produce());
		return;
	}

	//Get the valid operand types, check operator
	const DataType * lhsType = typing->nodeType(myExp1);
	const DataType * rhsType = typing->nodeType(myExp2);

	if (lhsType->isInt() && rhsType->isInt()){
		typing->nodeType(this, BasicType::INT());
		return;
	}
	if (lhsType->isByte() && rhsType->isInt()){
		ByteToIntNode * promote = new ByteToIntNode(myExp1);
		myExp1 = promote;
		typing->nodeType(promote, BasicType::INT());
		typing->nodeType(this, BasicType::INT());
		return;
	}
	if (lhsType->isInt() && rhsType->isByte()){
		ByteToIntNode * promote = new ByteToIntNode(myExp2);
		myExp2 = promote;
		typing->nodeType(promote, BasicType::INT());
		typing->nodeType(this, BasicType::INT());
		return;
	}
	if (lhsType->isByte() && rhsType->isByte()){
		typing->nodeType(this, BasicType::BYTE());
		return;
	}

	typing->nodeType(this, ErrorType::produce());
	return;
}

static const DataType * typeLogicOpd(
	TypeAnalysis * typing, ExpNode * opd
){
	opd->typeAnalysis(typing);
	const DataType * type = typing->nodeType(opd);

	//Return type if it's valid
	if (type->isBool()){ return type; }

	//Don't re-report an error, but return null to
	// indicate incompatibility
	if (type->asError()){ return nullptr; }

	//If type isn't an error, but is incompatible,
	// report and indicate incompatibility
	typing->errLogicOpd(opd->line(), opd->col());
	return NULL;
}

void BinaryExpNode::binaryLogicTyping(TypeAnalysis * typing){
	const DataType * lhsType = typeLogicOpd(typing, myExp1);
	const DataType * rhsType = typeLogicOpd(typing, myExp2);
	if (!lhsType || !rhsType){
		typing->nodeType(this, ErrorType::produce());
		return;
	}

	//Given valid operand types, check operator
	if (lhsType->isBool() && rhsType->isBool()){
		typing->nodeType(this, BasicType::BOOL());
		return;
	}

	//We never expect to get here, so we'll consider it
	// an error with the compiler itself
	throw new InternalError("Incomplete typing");
	typing->nodeType(this, ErrorType::produce());
	return;
}

void PlusNode::typeAnalysis(TypeAnalysis * typing){
	binaryMathTyping(typing);
}

void MinusNode::typeAnalysis(TypeAnalysis * typing){
	binaryMathTyping(typing);
}

void TimesNode::typeAnalysis(TypeAnalysis * typing){
	binaryMathTyping(typing);
}

void DivideNode::typeAnalysis(TypeAnalysis * typing){
	binaryMathTyping(typing);
}

void AndNode::typeAnalysis(TypeAnalysis * typing){
	binaryLogicTyping(typing);
}

void OrNode::typeAnalysis(TypeAnalysis * typing){
	binaryLogicTyping(typing);
}

static const DataType * typeEqOpd(
	TypeAnalysis * typing, ExpNode * opd
){
	opd->typeAnalysis(typing);
	const DataType * type = typing->nodeType(opd);

	if (type->isInt()){ return type; }
	if (type->isBool()){ return type; }
	if (type->isByte()){ return type; }

	//Arrays are not allowed in equality testing
	//if (type->isArray()){ return type; }

	//Errors are invalid, but don't cause re-reports
	if (type->asError()){ return nullptr; }

	typing->errEqOpd(opd->line(), opd->col());
	return nullptr;
}

void BinaryExpNode::binaryEqTyping(TypeAnalysis * typing){
	const DataType * lhsType = typeEqOpd(typing, myExp1);
	const DataType * rhsType = typeEqOpd(typing, myExp2);

	if (!lhsType || !rhsType){
		typing->nodeType(this, ErrorType::produce());
		return;
	}

	if (lhsType == rhsType){
		typing->nodeType(this, BasicType::BOOL());
		return;
	}

	const ArrayType * lhsArr = lhsType->asArray();
	const ArrayType * rhsArr = rhsType->asArray();
	if (lhsArr && rhsArr){
		/*
		if (lhsArr->baseType() == rhsArr->baseType()){
			typing->nodeType(this, BasicType::BOOL());
		} else {
			typing->errEqOpr(line(), col());
			typing->nodeType(this,ErrorType::produce());
		}
		*/
		return;
	}

	if (lhsType->isInt() && rhsType->isByte()){
		myExp2 = new ByteToIntNode(myExp2);
		typing->nodeType(myExp2, BasicType::INT());
		typing->nodeType(this, BasicType::BOOL());
		return;
	}
	if (lhsType->isByte() && rhsType->isInt()){
		myExp1 = new ByteToIntNode(myExp1);
		typing->nodeType(myExp1, BasicType::INT());
		typing->nodeType(this, BasicType::BOOL());
		return;
	}

	typing->errEqOpr(line(), col());
	typing->nodeType(this, ErrorType::produce());
	return;
}

void EqualsNode::typeAnalysis(TypeAnalysis * typing){
	binaryEqTyping(typing);
	assert(typing->nodeType(this) != nullptr);
}

void NotEqualsNode::typeAnalysis(TypeAnalysis * typing){
	binaryEqTyping(typing);
}

static const DataType * typeRelOpd(
	TypeAnalysis * typing, ExpNode * opd
){
	opd->typeAnalysis(typing);
	const DataType * type = typing->nodeType(opd);

	if (type->isInt()){ return type; }
	if (type->isByte()){ return type; }

	//Errors are invalid, but don't cause re-reports
	if (type->asError()){ return nullptr; }

	typing->errRelOpd(opd->line(),opd->col());
	typing->nodeType(opd, ErrorType::produce());
	return nullptr;
}

void BinaryExpNode::binaryRelTyping(TypeAnalysis * typing){
	const DataType * lhsType = typeRelOpd(typing, myExp1);
	const DataType * rhsType = typeRelOpd(typing, myExp2);

	if (!lhsType || !rhsType){
		typing->nodeType(this, ErrorType::produce());
		return;
	}

	if (lhsType->isInt() && rhsType->isInt()){
		typing->nodeType(this, BasicType::BOOL());
		return;
	}
	if (lhsType->isByte() && rhsType->isInt()){
		myExp1 = new ByteToIntNode(myExp1);
		typing->nodeType(myExp1, BasicType::INT());
		typing->nodeType(this, BasicType::BOOL());
	}
	if (lhsType->isInt() && rhsType->isByte()){
		myExp2 = new ByteToIntNode(myExp2);
		typing->nodeType(myExp2, BasicType::INT());
		typing->nodeType(this, BasicType::BOOL());
	}
	if (lhsType->isByte() && rhsType->isByte()){
		typing->nodeType(this, BasicType::BOOL());
	}

	//There is no bad relational operator, so we never 
	// expect to get here
	return;
}

void GreaterNode::typeAnalysis(TypeAnalysis * typing){
	binaryRelTyping(typing);
}

void GreaterEqNode::typeAnalysis(TypeAnalysis * typing){
	binaryRelTyping(typing);
}

void LessNode::typeAnalysis(TypeAnalysis * typing){
	binaryRelTyping(typing);
}

void LessEqNode::typeAnalysis(TypeAnalysis * typing){
	binaryRelTyping(typing);
}

void AssignStmtNode::typeAnalysis(TypeAnalysis * typing){
	myExp->typeAnalysis(typing);
	const DataType * childType = typing->nodeType(myExp);
	if (childType->asError()){
		typing->nodeType(this, ErrorType::produce());
	} else {
		typing->nodeType(this, BasicType::VOID());
	}
}

void PostDecStmtNode::typeAnalysis(TypeAnalysis * typing){
	myLVal->typeAnalysis(typing);
	const DataType * childType = typing->nodeType(myLVal);

	if (childType->asError()){ return; }
	if (childType->isInt()){ return; }
	if (childType->isByte()){ return; }

	//Any other unary math is an error
	typing->errMathOpd(myLVal->line(), myLVal->col());
}

void PostIncStmtNode::typeAnalysis(TypeAnalysis * typing){
	myLVal->typeAnalysis(typing);
	const DataType * childType = typing->nodeType(myLVal);

	if (childType->asError()){ return; }
	if (childType->isInt()){ return; }
	if (childType->isByte()){ return; }

	//Any other unary math is an error
	typing->errMathOpd(myLVal->line(), myLVal->col());
}

void ReadStmtNode::typeAnalysis(TypeAnalysis * typing){
	myDst->typeAnalysis(typing);
	const DataType * childType = typing->nodeType(myDst);
	const BasicType * childAsVar = childType->asBasic();

	if (childType->isBool()){
		return;
	} else if (childType->isInt()){
		return;
	} else if (childType->isArray()){
		const ArrayType * arr = childType->asArray();
		if (arr->baseType()->isByte()){
			return;
		}
	} else if (childType->asFn()){
		typing->errReadFn(myDst->line(),myDst->col());
		typing->nodeType(this, ErrorType::produce());
		return;
	} else {
		typing->errReadOther(myDst->line(),myDst->col());
	}
	typing->nodeType(this, BasicType::VOID());
}

void WriteStmtNode::typeAnalysis(TypeAnalysis * typing){
	mySrc->typeAnalysis(typing);
	const DataType * childType = typing->nodeType(mySrc);

	//Mark error, but don't re-report
	if (childType->asError()){
		typing->nodeType(this, ErrorType::produce());
		return;
	}

	//Check for invalid type
	if (childType->isVoid()){
		typing->errWriteVoid(mySrc->line(), mySrc->col());
		typing->nodeType(this, ErrorType::produce());
		return;
	} else if (childType->asFn()){
		typing->errWriteFn(mySrc->line(), mySrc->col());
		typing->nodeType(this, ErrorType::produce());
		return;
	} else if (childType->asBasic()){
		//Can write to a var of any other type
		return;
	} else if (const ArrayType * arr = childType->asArray()){
		if(arr->baseType()->isByte()){
			return;
		} else {
			typing->errWriteArray(mySrc->line(), mySrc->col());
			return;
		}
	}

	/*
	if (const PtrType * asPtr = childType->asPtr()){
		//TODO: FIXME
		const DataType * deref = PtrType::derefType(asPtr);
		const BasicType * base = deref->asBasic();
		assert(base != nullptr);
			
		if (base->isByte()){
			typing->nodeType(this, BasicType::VOID());
		} else {
			size_t line = mySrc->line();
			size_t col = mySrc->line();
			typing->badWritePtr(line, col);
		}
		return;
	}
	*/

	typing->nodeType(this, BasicType::VOID());
}

void IfStmtNode::typeAnalysis(TypeAnalysis * typing){
	//Start off the typing as void, but may update to error
	typing->nodeType(this, BasicType::VOID());

	myCond->typeAnalysis(typing);
	const DataType * condType = typing->nodeType(myCond);
	bool goodCond = true;
	if (condType->asError()){
		typing->nodeType(this, ErrorType::produce());
		goodCond = false;
	} else if (!condType->isBool()){
		goodCond = false;
		typing->errIfCond(
			myCond->line(), 
			myCond->col());
		typing->nodeType(this, 
			ErrorType::produce());
	}

	for (auto stmt : *myBody){
		stmt->typeAnalysis(typing);
	}

	if (goodCond){
		typing->nodeType(this, BasicType::produce(VOID));
	} else {
		typing->nodeType(this, ErrorType::produce());
	}
}

void IfElseStmtNode::typeAnalysis(TypeAnalysis * typing){
	myCond->typeAnalysis(typing);
	const DataType * condType = typing->nodeType(myCond);

	bool goodCond = true;
	if (condType->asError()){
		goodCond = false;
		typing->nodeType(this, ErrorType::produce());
	} else if (!condType->isBool()){
		typing->errIfCond(myCond->line(), myCond->col());
		goodCond = false;
	}
	for (auto stmt : *myBodyTrue){
		stmt->typeAnalysis(typing);
	}
	for (auto stmt : *myBodyFalse){
		stmt->typeAnalysis(typing);
	}
	
	if (goodCond){
		typing->nodeType(this, BasicType::produce(VOID));
	} else {
		typing->nodeType(this, ErrorType::produce());
	}
}

void WhileStmtNode::typeAnalysis(TypeAnalysis * typing){
	myCond->typeAnalysis(typing);
	const DataType * condType = typing->nodeType(myCond);

	if (condType->asError()){
		typing->nodeType(this, ErrorType::produce());
	} else if (!condType->isBool()){
		typing->errWhileCond(myCond->line(), myCond->col());
	}

	for (auto stmt : *myBody){
		stmt->typeAnalysis(typing);
	}

	typing->nodeType(this, BasicType::VOID());
}

void CallStmtNode::typeAnalysis(TypeAnalysis * typing){
	myCallExp->typeAnalysis(typing);
	typing->nodeType(this, BasicType::VOID());
}

void ReturnStmtNode::typeAnalysis(TypeAnalysis * typing){
	const FnType * fnType = typing->getCurrentFnType();
	const DataType * fnRet = fnType->getReturnType();

	//Check: shouldn't return anything
	if (fnRet == BasicType::VOID()){
		if (myExp != nullptr) {
			myExp->typeAnalysis(typing);
			typing->extraRetValue(
				myExp->line(), 
				myExp->col()); 
			typing->nodeType(this, ErrorType::produce());
		} else {
			typing->nodeType(this, BasicType::VOID());
		}
		return;
	}

	//Check: returns nothing, but should
	if (myExp == nullptr){
			typing->errRetEmpty(line(), col());
			typing->nodeType(this, ErrorType::produce());
			return;
	}

	myExp->typeAnalysis(typing);
	const DataType * childType = typing->nodeType(myExp);

	if (childType->isByte() && fnRet->isInt()){
		//Promote
		myExp = new ByteToIntNode(myExp);
		typing->nodeType(myExp, BasicType::INT());
		return;
	}

	if (childType->asError()){
		typing->nodeType(this, ErrorType::produce());
		return;
	}

	if (childType != fnRet){
		typing->errRetWrong(myExp->line(), myExp->col());
		typing->nodeType(this, ErrorType::produce());
		return;
	}
	typing->nodeType(this, ErrorType::produce());
	return;
}

void StrLitNode::typeAnalysis(TypeAnalysis * typing){
	BasicType * basic = BasicType::BYTE();
	ArrayType * asArr = ArrayType::produce(basic, 0);
	typing->nodeType(this, asArr);
}

void HavocNode::typeAnalysis(TypeAnalysis * typing){
	typing->nodeType(this, BasicType::BOOL());
}

void FalseNode::typeAnalysis(TypeAnalysis * typing){
	typing->nodeType(this, BasicType::BOOL());
}

void TrueNode::typeAnalysis(TypeAnalysis * typing){
	typing->nodeType(this, BasicType::BOOL());
}

void IntLitNode::typeAnalysis(TypeAnalysis * typing){
	if (myNum < 256){
		typing->nodeType(this, BasicType::BYTE());
	} else {
		typing->nodeType(this, BasicType::INT());
	}
}

}
