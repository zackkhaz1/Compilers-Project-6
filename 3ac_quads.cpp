#include "3ac.hpp"

namespace crona{


Quad::Quad() : myComment(""){
}

void Quad::addLabel(Label * label){
	if (label != nullptr){
		labels.push_back(label);
	}
}

void Quad::setComment(std::string commentIn){
	this->myComment = commentIn;
}

std::string Quad::commentStr(){
	if (myComment.length() > 0){
		return "  #" + myComment;
	}
	return "";
}

std::string Quad::toString(bool verbose){
	auto res = std::string("");

	auto first = true;

	size_t labelSpace = 12;
	for (auto label : labels){
		if (first){ first = false; }
		else { res += ","; }

		res += label->toString();
	}
	if (!first){ res += ": "; }
	else { res += "  "; }
	size_t spaces;
	if (res.length() > labelSpace){ spaces = 0; }
	else { spaces = labelSpace - res.length(); }
	for (size_t i = 0; i < spaces; i++){
		res += " ";
	}

	res += this->repr();
	if (verbose){
		res += commentStr();
	}

	return res;
}

CallQuad::CallQuad(SemSymbol * calleeIn) : callee(calleeIn){ }

std::string CallQuad::repr(){
	return "call " + callee->getName();
}

EnterQuad::EnterQuad(Procedure * procIn)
: Quad(), myProc(procIn) { }

std::string EnterQuad::repr(){
	return "enter " + myProc->getName();
}

LeaveQuad::LeaveQuad(Procedure * procIn)
: Quad(), myProc(procIn) { }

std::string LeaveQuad::repr(){
	return "leave " + myProc->getName();
}

std::string AssignQuad::repr(){
	return dst->valString() + " := " + src->valString();
	
}

AssignQuad::AssignQuad(Opd * dstIn, Opd * srcIn): dst(dstIn), src(srcIn){
	assert(dstIn != nullptr);
	assert(srcIn != nullptr);
}

BinOpQuad::BinOpQuad(Opd * dstIn, BinOp oprIn, Opd * src1In, Opd * src2In)
: dst(dstIn), opr(oprIn), src1(src1In), src2(src2In){
	assert(dstIn != nullptr);
	assert(src1In != nullptr);
	assert(src2In != nullptr);
}

std::string BinOpQuad::oprString(BinOp opr){
	switch(opr){
	case ADD8: return "ADD8";  
	case ADD64: return "ADD64";  
	case SUB8: return "SUB8";  
	case SUB64: return "SUB64";  
	case DIV8: return "DIV8";  
	case DIV64: return "DIV64";  
	case MULT8: return "MULT8";  
	case MULT64: return "MULT64";  
	case OR8: return "OR8";  
	case AND8: return "AND8";  
	case EQ8: return "EQ8";  
	case EQ64: return "EQ64";  
	case NEQ8: return "NEQ8";  
	case NEQ64: return "NEQ64";  
	case LT8: return "LT8";  
	case LT64: return "LT64";  
	case GT8: return "GT8";  
	case GT64: return "GT64";  
	case LTE8: return "LTE8";  
	case LTE64: return "LTE64";  
	case GTE8: return "GTE8";  
	case GTE64: return "GTE64";  	
	} 
	return " ";
}

std::string BinOpQuad::repr(){
	std::string opString;
	return dst->valString()
		+ " := " 
		+ src1->valString()
		+ " " + BinOpQuad::oprString(opr) + " "
		+ src2->valString();
}

UnaryOpQuad::UnaryOpQuad(Opd * dstIn, UnaryOp opIn, Opd * srcIn)
: dst(dstIn), op(opIn), src(srcIn) { 
	assert(dstIn != nullptr);
	assert(srcIn != nullptr);
}

std::string UnaryOpQuad::repr(){
	std::string opString;
	switch (op){
	case NEG64:
		opString = "NEG64 ";
		break;
	case NOT8:
		opString = "NOT8 ";
	}
	return dst->valString() + " := " 
		+ opString
		+ src->valString();
}

HavocQuad::HavocQuad(Opd * dstIn): myDst(dstIn){}

std::string HavocQuad::repr(){
	return "HAVOC " + myDst->valString();
}

WriteQuad::WriteQuad(Opd * opd, const DataType * type) 
: myArg(opd), myType(type){ }

std::string WriteQuad::repr(){
	return "WRITE " + myArg->valString();
}

ReadQuad::ReadQuad(Opd * opd, const DataType * type)
: myArg(opd), myType(type){ }

std::string ReadQuad::repr(){
	return "READ " + myArg->valString();
}

JmpQuad::JmpQuad(Label * tgtIn)
: Quad(), tgt(tgtIn){ }

std::string JmpQuad::repr(){
	std::string res = "";
	return "goto " + tgt->toString();
}

JmpIfQuad::JmpIfQuad(Opd * cndIn, Label * tgtIn) 
: Quad(), cnd(cndIn), tgt(tgtIn){ }

std::string JmpIfQuad::repr(){
	std::string res = "IFZ ";
	res += cnd->valString();
	res += " GOTO ";
	res += tgt->toString();
	return res;
}

NopQuad::NopQuad()
: Quad() { }

std::string NopQuad::repr(){
	return "nop";
}

GetRetQuad::GetRetQuad(Opd * opdIn)
: Quad(), opd(opdIn) { }

std::string GetRetQuad::repr(){
	std::string res = "";
	res += "getret " + opd->valString(); 
	return res;
}

SetArgQuad::SetArgQuad(size_t indexIn, Opd * opdIn) 
: index(indexIn), opd(opdIn){
}

std::string SetArgQuad::repr(){
	std::string res = "";
	res += "setarg " + std::to_string(index) + " " + opd->valString(); 
	return res;
}

GetArgQuad::GetArgQuad(size_t indexIn, Opd * opdIn) 
: index(indexIn), opd(opdIn){
}

std::string GetArgQuad::repr(){
	std::string res = "";
	res += "getarg " + std::to_string(index) + " " + opd->valString(); 
	return res;
}

SetRetQuad::SetRetQuad(Opd * opdIn) 
: opd(opdIn){
}

std::string SetRetQuad::repr(){
	std::string res = "";
	res += "setret " + opd->valString(); 
	return res;
}

std::string IndexQuad::repr(){
	std::string res = dst->locString() + " := "
	+ src->locString() + " ADD64 " + off->valString();
	return res;
}

}
