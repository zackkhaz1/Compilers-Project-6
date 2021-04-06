#include "ast.hpp"

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
	q->setComment("AssignExp");
	proc->addQuad(q);
	return(left);
}

Opd * LValNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * CallExpNode::flatten(Procedure * proc){
	std::list<Opd*> opdList;
	for(auto arg : *myArgs)
	{
		opdList.push_back(arg->flatten(proc));
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
		proc->addQuad(retQ);
		return(ret);

	}

	return(ret);
}

Opd * ByteToIntNode::flatten(Procedure * proc){
	TODO(Implement me)
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
	Opd* dest = proc->makeTmp(8);
	Quad* q = new BinOpQuad(dest, ADD64, left, right);
	proc->addQuad(q);
	return dest;
}

Opd * MinusNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right =  myExp2->flatten(proc);
	Opd* dest = proc->makeTmp(8);
	Quad* q = new BinOpQuad(dest, NEQ64, left, right);
	proc->addQuad(q);
	return dest;
}

Opd * TimesNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right =  myExp2->flatten(proc);
	Opd* dest = proc->makeTmp(8);
	Quad* q = new BinOpQuad(dest, MULT64, left, right);
	proc->addQuad(q);
	return dest;
}

Opd * DivideNode::flatten(Procedure * proc){
	Opd* left = myExp1->flatten(proc);
	Opd* right =  myExp2->flatten(proc);
	Opd* dest = proc->makeTmp(8);
	Quad* q = new BinOpQuad(dest, DIV64, left, right);
	proc->addQuad(q);
	return dest;
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
	Opd* op1 = myExp1->flatten(proc);
	Opd* op2 = myExp2->flatten(proc);
	Opd* op3 = proc->makeTmp(8);
	Quad* q = new BinOpQuad(op3, EQ64, op1, op2);
	proc->addQuad(q);
	return op1;
}

Opd * NotEqualsNode::flatten(Procedure * proc){
	Opd* op1 = myExp1->flatten(proc);
	Opd* op2 = myExp2->flatten(proc);
	Opd* op3 = proc->makeTmp(8);
	Quad* q = new BinOpQuad(op3, LTE64, op1, op2);
	proc->addQuad(q);
	return op1;
}

Opd * LessNode::flatten(Procedure * proc){
	Opd* op1 = myExp1->flatten(proc);
	Opd* op2 = myExp2->flatten(proc);
	Opd* op3 = proc->makeTmp(8);
	Quad* q = new BinOpQuad(op3, LT64, op1, op2);
	proc->addQuad(q);
	return op1;
}

Opd * GreaterNode::flatten(Procedure * proc){
	Opd* op1 = myExp1->flatten(proc);
	Opd* op2 = myExp2->flatten(proc);
	Opd* op3 = proc->makeTmp(8);
	Quad* q = new BinOpQuad(op3, GT64, op1, op2);
	proc->addQuad(q);
	return op1;
}

Opd * LessEqNode::flatten(Procedure * proc){
	Opd* op1 = myExp1->flatten(proc);
	Opd* op2 = myExp2->flatten(proc);
	Opd* op3 = proc->makeTmp(8);
	Quad* q = new BinOpQuad(op3, LTE64, op1, op2);
	proc->addQuad(q);
	return op1;
}

Opd * GreaterEqNode::flatten(Procedure * proc){
	Opd* op1 = myExp1->flatten(proc);
	Opd* op2 = myExp2->flatten(proc);
	Opd* op3 = proc->makeTmp(8);
	Quad* q = new BinOpQuad(op3, GTE64, op1, op2);
	proc->addQuad(q);
	return op1;
}

void AssignStmtNode::to3AC(Procedure * proc){
	myExp->flatten(proc);
}

void PostIncStmtNode::to3AC(Procedure * proc){
	Opd* inc = myLVal->flatten(proc);
	LitOpd* literal = new LitOpd("1",8);
	BinOpQuad* binQuad = new BinOpQuad(inc, ADD8, inc, literal);
	proc->addQuad(binQuad);
}

void PostDecStmtNode::to3AC(Procedure * proc){
	Opd* inc = myLVal->flatten(proc);
	LitOpd* literal = new LitOpd("1",8);
	BinOpQuad* binQuad = new BinOpQuad(inc, SUB8, inc, literal);
	proc->addQuad(binQuad);
}

void ReadStmtNode::to3AC(Procedure * proc){
	////Not sure if this is right
	Opd* dest = myDst->flatten(proc);
	//ReadQuad* rQuad = new ReadQuad(dest, READ);
	//proc->addQuad(rQuad);
}

void WriteStmtNode::to3AC(Procedure * proc){
	////Not sure if this is right
	Opd* src = mySrc->flatten(proc);
	//WriteQuad* wQuad = new WriteQuad(src, WRITE);
	//proc->addQuad(wQuad);
}

void IfStmtNode::to3AC(Procedure * proc){
	TODO(Implement me)
}

void IfElseStmtNode::to3AC(Procedure * proc){
	TODO(Implement me)
}

void WhileStmtNode::to3AC(Procedure * proc){
	TODO(Implement me)
}

void CallStmtNode::to3AC(Procedure * proc){
	TODO(Implement me)
}

void ReturnStmtNode::to3AC(Procedure * proc){
	TODO(Implement me)
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
	TODO(Implement me)
}

//We only get to this node if we are in a stmt
// context (DeclNodes protect descent)
Opd * IDNode::flatten(Procedure * proc){
	TODO(Implement me)
}

}
