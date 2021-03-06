#include "ast.hpp"
#include <iostream>

namespace crona{

IRProgram * ProgramNode::to3AC(TypeAnalysis * ta){
	IRProgram * prog = new IRProgram(ta);
	for (auto global : *myGlobals){
		global->to3AC(prog);
	}
	return prog;
}

void FnDeclNode::to3AC(IRProgram * prog){
	Procedure * p = prog->makeProc(myID->getName());

	for(auto f : *myFormals){
		f->to3AC(p);
	}

	size_t idx = 1;
	std::list<SymOpd *> formals = p->getFormals();
	for(auto opd : formals)
	{
		Quad* arg = new GetArgQuad(idx, opd);
		p->addQuad(arg);
		idx++;
	}

	for(auto b : *myBody){
		b->to3AC(p);
	}

}

void FnDeclNode::to3AC(Procedure * proc){
	//This never needs to be implemented,
	// the function only exists because of
	// inheritance needs (A function declaration
	// never occurs within another function)
	throw new InternalError("FnDecl at a local scope");
}

void FormalDeclNode::to3AC(IRProgram * prog){
	//This never needs to be implemented,
	// the function only exists because of
	// inheritance needs (A formal never
	// occurs at global scope)
	throw new InternalError("Formal at a global scope");
}

void FormalDeclNode::to3AC(Procedure * proc){
	SemSymbol* s = ID()->getSymbol();
	if(s == nullptr)
	{
		throw new InternalError("Invalid symbol");
	}
	proc->gatherFormal(s);
}

Opd * IntLitNode::flatten(Procedure * proc){
	const DataType * type = proc->getProg()->nodeType(this);
	if (type->isByte()){
		return new LitOpd(std::to_string(myNum), 1);
	} else {
		return new LitOpd(std::to_string(myNum), 8);
	}
}

Opd * StrLitNode::flatten(Procedure * proc){
	Opd * res = proc->getProg()->makeString(myStr);
	return res;
}

Opd * HavocNode::flatten(Procedure * proc){
	Opd * dst = proc->makeTmp(1);
	HavocQuad * havoc = new HavocQuad(dst);
	proc->addQuad(havoc);
	return dst;
}

Opd * TrueNode::flatten(Procedure * proc){
	Opd* tru = new LitOpd("1",1);
	return tru;
}

Opd * FalseNode::flatten(Procedure * proc){
	Opd* fals = new LitOpd("0",1);
	return fals;
}

Opd * AssignExpNode::flatten(Procedure * proc){
	Opd* right = mySrc->flatten(proc);
	Opd* left = myDst->flatten(proc);
	if(!left){
		throw InternalError("Invalid destination");
	}
	AssignQuad* q = new AssignQuad(left, right);
	proc->addQuad(q);
	return(left);
}

Opd * LValNode::flatten(Procedure * proc){
	TODO();
}

Opd * CallExpNode::flatten(Procedure * proc){
	std::list<Opd*> opdList;
	for(auto arg : *myArgs)
	{
		opdList.push_back(arg->flatten(proc));
	}

	size_t idx = 1;
	for(auto opd : opdList)
	{
		Quad* arg = new SetArgQuad(idx, opd);
		proc->addQuad(arg);
		idx++;
	}

	SemSymbol* fnIdentifier = myID->getSymbol();
	CallQuad* cQuad = new CallQuad(fnIdentifier);
	proc->addQuad(cQuad);

	Opd* ret = nullptr;
	DataType* returnType = fnIdentifier->getDataType();
	if(!returnType->isVoid())
	{
		ret = proc->makeTmp(8);
		GetRetQuad* retQ = new GetRetQuad(ret);
		return ret;
	}
	return ret;
}

Opd * ByteToIntNode::flatten(Procedure * proc){
	Opd* childOpd = myChild->flatten(proc);
	Opd* tempOpd = proc->makeTmp(8);
	Quad* q = new AssignQuad(tempOpd, childOpd); 
	proc->addQuad(q);
	return tempOpd;
}

Opd * NegNode::flatten(Procedure * proc){
	Opd* op1 = myExp->flatten(proc);
	Opd* op2 = proc->makeTmp(8);
	Quad* q = new UnaryOpQuad(op1, NEG64, op2);
	proc->addQuad(q);
	return op1;
}

Opd * NotNode::flatten(Procedure * proc){
	Opd* op1 = myExp->flatten(proc);
	Opd* op2 = proc->makeTmp(8);
	Quad* q = new UnaryOpQuad(op1, NOT8, op2);
	proc->addQuad(q);
	return op1;
}

Opd * PlusNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right =  myExp2->flatten(proc);
	auto leftSize = left->getWidth();
	auto rightSize = right->getWidth();
	if(leftSize > 1 && rightSize > 1){
		Opd* dest = proc->makeTmp(8);
		Quad* q = new BinOpQuad(dest, ADD64, left, right);
		proc->addQuad(q);
		return dest;
	} else {
		Opd* dest = proc->makeTmp(1);
		Quad* q = new BinOpQuad(dest, ADD8, left, right);
		proc->addQuad(q);
		return dest;
	}
}

Opd * MinusNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right =  myExp2->flatten(proc);
	auto leftSize = left->getWidth();
	auto rightSize = right->getWidth();
	if(leftSize > 1 && rightSize > 1){
		Opd* dest = proc->makeTmp(8);
		Quad* q = new BinOpQuad(dest, SUB64, left, right);
		proc->addQuad(q);
		return dest;
	} else {
		Opd* dest = proc->makeTmp(1);
		Quad* q = new BinOpQuad(dest, SUB8, left, right);
		proc->addQuad(q);
		return dest;
	}
}

Opd * TimesNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right =  myExp2->flatten(proc);
	auto leftSize = left->getWidth();
	auto rightSize = right->getWidth();
	if(leftSize > 1 && rightSize > 1){
		Opd* dest = proc->makeTmp(8);
		Quad* q = new BinOpQuad(dest, MULT64, left, right);
		proc->addQuad(q);
		return dest;
	} else {
		Opd* dest = proc->makeTmp(1);
		Quad* q = new BinOpQuad(dest, MULT8, left, right);
		proc->addQuad(q);
		return dest;
	}
}

Opd * DivideNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right =  myExp2->flatten(proc);
	auto leftSize = left->getWidth();
	auto rightSize = right->getWidth();
	if(leftSize > 1 && rightSize > 1){
		Opd* dest = proc->makeTmp(8);
		Quad* q = new BinOpQuad(dest, DIV64, left, right);
		proc->addQuad(q);
		return dest;
	} else {
		Opd* dest = proc->makeTmp(1);
		Quad* q = new BinOpQuad(dest, DIV8, left, right);
		proc->addQuad(q);
		return dest;
	}
}

Opd * AndNode::flatten(Procedure * proc){
	Opd* op1 = myExp1->flatten(proc);
	Opd* op2 = myExp2->flatten(proc);
	Opd* op3 = proc->makeTmp(8);
	Quad* q = new BinOpQuad(op3, AND8, op1, op2);
	proc->addQuad(q);
	return op1;
}

Opd * OrNode::flatten(Procedure * proc){
	Opd* op1 = myExp1->flatten(proc);
	Opd* op2 = myExp2->flatten(proc);
	Opd* op3 = proc->makeTmp(8);
	Quad* q = new BinOpQuad(op3, OR8, op1, op2);
	proc->addQuad(q);
	return op1;
}

Opd * EqualsNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right = myExp2->flatten(proc);
	auto leftSize = left->getWidth();
	auto rightSize = right->getWidth();
	if(leftSize > 1 && rightSize > 1){
		Opd* dest = proc->makeTmp(8);
		Quad* q = new BinOpQuad(dest, EQ64, left, right);
		proc->addQuad(q);
		return dest;
	} else {
		Opd* dest = proc->makeTmp(1);
		Quad* q = new BinOpQuad(dest, EQ8, left, right);
		proc->addQuad(q);
		return dest;
	}
}

Opd * NotEqualsNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right = myExp2->flatten(proc);
	auto leftSize = left->getWidth();
	auto rightSize = right->getWidth();
	if(leftSize > 1 && rightSize > 1){
		Opd* dest = proc->makeTmp(8);
		Quad* q = new BinOpQuad(dest, NEQ64, left, right);
		proc->addQuad(q);
		return dest;
	} else {
		Opd* dest = proc->makeTmp(1);
		Quad* q = new BinOpQuad(dest, NEQ8, left, right);
		proc->addQuad(q);
		return dest;
	}
}

Opd * LessNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right = myExp2->flatten(proc);
	auto leftSize = left->getWidth();
	auto rightSize = right->getWidth();
	if(leftSize > 1 && rightSize > 1){
		Opd* dest = proc->makeTmp(8);
		Quad* q = new BinOpQuad(dest, LT64, left, right);
		proc->addQuad(q);
		return dest;
	} else {
		Opd* dest = proc->makeTmp(1);
		Quad* q = new BinOpQuad(dest, LT8, left, right);
		proc->addQuad(q);
		return dest;
	}
}

Opd * GreaterNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right = myExp2->flatten(proc);
	auto leftSize = left->getWidth();
	auto rightSize = right->getWidth();
	if(leftSize > 1 && rightSize > 1){
		Opd* dest = proc->makeTmp(8);
		Quad* q = new BinOpQuad(dest, GT64, left, right);
		proc->addQuad(q);
		return dest;
	} else {
		Opd* dest = proc->makeTmp(1);
		Quad* q = new BinOpQuad(dest, GT8, left, right);
		proc->addQuad(q);
		return dest;
	}
}

Opd * LessEqNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right = myExp2->flatten(proc);
	auto leftSize = left->getWidth();
	auto rightSize = right->getWidth();
	if(leftSize > 1 && rightSize > 1){
		Opd* dest = proc->makeTmp(8);
		Quad* q = new BinOpQuad(dest, LTE64, left, right);
		proc->addQuad(q);
		return dest;
	} else {
		Opd* dest = proc->makeTmp(1);
		Quad* q = new BinOpQuad(dest, LTE8, left, right);
		proc->addQuad(q);
		return dest;
	}
}

Opd * GreaterEqNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right = myExp2->flatten(proc);
	auto leftSize = left->getWidth();
	auto rightSize = right->getWidth();
	if(leftSize > 1 && rightSize > 1){
		Opd* dest = proc->makeTmp(8);
		Quad* q = new BinOpQuad(dest, GTE64, left, right);
		proc->addQuad(q);
		return dest;
	} else {
		Opd* dest = proc->makeTmp(1);
		Quad* q = new BinOpQuad(dest, GTE8, left, right);
		proc->addQuad(q);
		return dest;
	}
}

void AssignStmtNode::to3AC(Procedure * proc){
	myExp->flatten(proc);
}

void PostIncStmtNode::to3AC(Procedure * proc){
	Opd* inc = myLVal->flatten(proc);
	LitOpd* literal = new LitOpd("1",8);
	BinOpQuad* binQuad = new BinOpQuad(inc, ADD64, inc, literal);
	proc->addQuad(binQuad);
}

void PostDecStmtNode::to3AC(Procedure * proc){
	Opd* inc = myLVal->flatten(proc);
	LitOpd* literal = new LitOpd("1",8);
	BinOpQuad* binQuad = new BinOpQuad(inc, SUB64, inc, literal);
	proc->addQuad(binQuad);
}

void ReadStmtNode::to3AC(Procedure * proc){
	Opd* dest = myDst->flatten(proc);
	auto type = proc->getProg()->nodeType(myDst);
	ReadQuad* rQuad = new ReadQuad(dest, type);
	proc->addQuad(rQuad);
}

void WriteStmtNode::to3AC(Procedure * proc){
	Opd* src = mySrc->flatten(proc);
	auto type = proc->getProg()->nodeType(mySrc);
	WriteQuad* wQuad = new WriteQuad(src, type);
	proc->addQuad(wQuad);
}

void IfStmtNode::to3AC(Procedure * proc){
	Opd* condOpd = myCond->flatten(proc);
	Label* exitIf = proc->makeLabel();
	Quad* jumpIf = new JmpIfQuad(condOpd, exitIf);
	proc->addQuad(jumpIf);
	for (auto stmt: *myBody){
		stmt->to3AC(proc);
	}
	Quad* exit = new NopQuad();
	exit->addLabel(exitIf);
	proc->addQuad(exit);
}

void IfElseStmtNode::to3AC(Procedure * proc){
	Opd* condOpd = myCond->flatten(proc);
	Label* elseLbl = proc->makeLabel();
	Quad* jumpElse = new JmpIfQuad(condOpd, elseLbl);
	proc->addQuad(jumpElse);
	
	for (auto stmt: *myBodyTrue){
		stmt->to3AC(proc);
	}
	
	Label* exitIfElse = proc->makeLabel();
	Quad* skipElse = new JmpQuad(exitIfElse);
	proc->addQuad(skipElse);

	Quad* elseNop = new NopQuad();
	elseNop->addLabel(elseLbl);
	proc->addQuad(elseNop);

	for (auto stmt : *myBodyFalse){
		stmt->to3AC(proc);
	}

	Quad* exit = new NopQuad();
	exit->addLabel(exitIfElse);
	proc->addQuad(exit);
}

void WhileStmtNode::to3AC(Procedure * proc){
	Label* loopStart = proc->makeLabel();
	Quad* start = new NopQuad();
	start->addLabel(loopStart);
	proc->addQuad(start);

	Opd* condOpd = myCond->flatten(proc);
	Label* exitWhile = proc->makeLabel();
	Quad* falseCondJump = new JmpIfQuad(condOpd, exitWhile);

	for (auto stmt : *myBody){
		stmt->to3AC(proc);
	}

	Quad* loopBack = new JmpQuad(loopStart);
	proc->addQuad(loopBack);
	Quad* exit = new NopQuad();
	exit->addLabel(exitWhile);
	proc->addQuad(exit);
}

void CallStmtNode::to3AC(Procedure * proc){
	myCallExp->flatten(proc);
}

void ReturnStmtNode::to3AC(Procedure * proc){
	if(myExp != NULL){
		Opd* childReturn = myExp->flatten(proc);
		Quad* setRet = new SetRetQuad(childReturn);
		proc->addQuad(setRet);
	}
	Quad* jump = new JmpQuad(proc->getLeaveLabel());
	proc->addQuad(jump);
}

void VarDeclNode::to3AC(Procedure * proc){
	SemSymbol * sym = ID()->getSymbol();
	if (sym == nullptr){
		throw new InternalError("null sym");
	}
	proc->gatherLocal(sym);
}

void VarDeclNode::to3AC(IRProgram * prog){
	SemSymbol * sym = ID()->getSymbol();
	if (sym == nullptr){
		throw new InternalError("null sym");
	}

	prog->gatherGlobal(sym);
}

Opd * IndexNode::flatten(Procedure * proc){
	Opd* idxOpd = myOffset->flatten(proc);
	Opd* idOpd = myBase->flatten(proc);
	auto type = proc->getProg()->nodeType(this);
	if (type->isByte() || type->isBool()){
		auto tmpAddr = proc->makeAddrOpd(1);
		Quad* idxQuad = new IndexQuad(tmpAddr, idOpd, idxOpd);
		proc->addQuad(idxQuad);
		return tmpAddr;
	}
	auto width = Opd::width(myBase->getSymbol()->getDataType());
	Opd* tmp = proc->makeTmp(8);
	Opd* widthOpd = new LitOpd("8", width);
	Quad* q = new BinOpQuad(tmp, MULT64, idxOpd, widthOpd);
	proc->addQuad(q);
	auto tmpAddr = proc->makeAddrOpd(8);
	Quad* idxQuad = new IndexQuad(tmpAddr, idOpd, tmp);
	proc->addQuad(idxQuad);
	return tmpAddr;
}

//We only get to this node if we are in a stmt
// context (DeclNodes protect descent)
Opd * IDNode::flatten(Procedure * proc){
	Opd* sym = proc->getSymOpd(mySymbol);
	if(sym == NULL){
		throw new InternalError("null ID sym");
	}
	return sym;
}

}
