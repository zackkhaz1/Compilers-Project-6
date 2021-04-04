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
	TODO(Implement me)
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
	TODO(Implement me)
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
	TODO(Implement me)
}

Opd * FalseNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * AssignExpNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * LValNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * CallExpNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * ByteToIntNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * NegNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * NotNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * PlusNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * MinusNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * TimesNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * DivideNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * AndNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * OrNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * EqualsNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * NotEqualsNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * LessNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * GreaterNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * LessEqNode::flatten(Procedure * proc){
	TODO(Implement me)
}

Opd * GreaterEqNode::flatten(Procedure * proc){
	TODO(Implement me)
}

void AssignStmtNode::to3AC(Procedure * proc){
	TODO(Implement me)
}

void PostIncStmtNode::to3AC(Procedure * proc){
	TODO(Implement me)
}

void PostDecStmtNode::to3AC(Procedure * proc){
	TODO(Implement me)
}

void ReadStmtNode::to3AC(Procedure * proc){
	TODO(Implement me)
}

void WriteStmtNode::to3AC(Procedure * proc){
	TODO(Implement me)
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
