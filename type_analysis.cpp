#include "ast.hpp"
#include "symbol_table.hpp"
#include "errors.hpp"
#include "types.hpp"
#include "name_analysis.hpp"
#include "type_analysis.hpp"

namespace holeyc{

TypeAnalysis * TypeAnalysis::build(NameAnalysis * nameAnalysis){
	//To emphasize that type analysis depends on name analysis
	// being complete, a name analysis must be supplied for 
	// type analysis to be performed.
	TypeAnalysis * typeAnalysis = new TypeAnalysis();
	auto ast = nameAnalysis->ast;	
	typeAnalysis->ast = ast;

	ast->typeAnalysis(typeAnalysis);
	if (typeAnalysis->hasError){
		return nullptr;
	}

	return typeAnalysis;

}

void ProgramNode::typeAnalysis(TypeAnalysis * ta){

	//pass the TypeAnalysis down throughout
	// the entire tree, getting the types for
	// each element in turn and adding them
	// to the ta object's hashMap
	for (auto global : *myGlobals){
		global->typeAnalysis(ta);
	}

	//The type of the program node will never
	// be needed. We can just set it to VOID
	//(Alternatively, we could make our type 
	// be error if the DeclListNode is an error)
	ta->nodeType(this, BasicType::produce(VOID));
}

void FnDeclNode::typeAnalysis(TypeAnalysis * ta){ 
	DataType * ret_type = this->getRetTypeNode()->getType();

	std::list<const DataType*>* temp = new std::list<const DataType*>();
	for (auto decl : *myFormals){
		auto dt = decl->getTypeNode()->getType();
		const DataType *dt_const = const_cast<DataType*>(dt);
		temp->push_back(dt_const);
	}

    const std::list<const DataType*>* formals_list = const_cast<std::list<const DataType*>*>(temp);
	FnType *fn_type = new FnType(formals_list, ret_type);

	ta->setCurrentFnType(fn_type);
	ta->nodeType(this,fn_type);
}

void StmtNode::typeAnalysis(TypeAnalysis * ta){
	TODO("Implement me in the subclass");
}

void AssignStmtNode::typeAnalysis(TypeAnalysis * ta){ // IS THIS COMPELTE?
	
	// THIS MIGHT BE TEMPLATE FOR ALL PARENT NODES
	// 1. RUN CHILD TYPEANALYSIS
	// 2. IF CHILD IS ERORR, RETURN AS ERROR (AKA THE CHILDS TYPE)
	// 3. ELSE WE RETURN VOID SINCE THIS IS USELESS 
	// I WOULD THINK THIS NEEDS A TYPE THO? BC IF ITS USED IN A LESS THAN CASE?
		// NOT SURE WHY IT IS A VOID GIVEN:
			// int a;
			// int b;
			// b = a = (5 + 4); ??

	myExp->typeAnalysis(ta);

	//It can be a bit of a pain to write 
	// "const DataType *" everywhere, so here
	// the use of auto is used instead to tell the
	// compiler to figure out what the subType variable
	// should be
	auto subType = ta->nodeType(myExp);

	// As error returns null if subType is NOT an error type
	// otherwise, it returns the subType itself
	if (subType->asError()){
		ta->nodeType(this, subType);
	} else {
		ta->nodeType(this, BasicType::produce(VOID));
	}
}

void ExpNode::typeAnalysis(TypeAnalysis * ta){
	TODO("Override me in the subclass");
}

void AssignExpNode::typeAnalysis(TypeAnalysis * ta){ // I THINK THIS IS GOOD
	//TODO: Note that this function is incomplete. 
	// and needs additional code

	//Do typeAnalysis on the subexpressions
	myDst->typeAnalysis(ta);
	mySrc->typeAnalysis(ta);

	const DataType * tgtType = ta->nodeType(myDst);
	const DataType * srcType = ta->nodeType(mySrc);

	//While incomplete, this gives you one case for 
	// assignment: if the types are exactly the same
	// it is usually ok to do the assignment. One
	// exception is that if both types are function
	// names, it should fail type analysis
	
	// ACCOUNTING FOR INCORRECT TYPES

	if (((tgtType->asFn() != nullptr) && (srcType->asFn() != nullptr))) {
		ta->badAssignOpd(myDst->line(), myDst->col());
		ta->badAssignOpd(mySrc->line(), mySrc->col());
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	
	if (tgtType->asError()) {
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	else if(tgtType == BasicType::produce(VOID)) {
		ta->badAssignOpd(myDst->line(), myDst->col());
		ta->nodeType(this, ErrorType::produce());
	}

	if (srcType->asError()) {		
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	else if (srcType == BasicType::produce(VOID)) {
		ta->badAssignOpd(mySrc->line(), mySrc->col());
		ta->nodeType(this, ErrorType::produce());
	}
}

void DeclNode::typeAnalysis(TypeAnalysis * ta){
	TODO("Override me in the subclass");
}

void VarDeclNode::typeAnalysis(TypeAnalysis * ta){
	// VarDecls always pass type analysis, since they 
	// are never used in an expression. You may choose
	// to type them void (like this), as discussed in class
	ta->nodeType(this, BasicType::produce(VOID));
}

void FormalDeclNode::typeAnalysis(TypeAnalysis * ta){ // NOT SURE IF WE NEED THIS
	ta->nodeType(this, BasicType::produce(VOID));
}

void IDNode::typeAnalysis(TypeAnalysis * ta){
	// IDs never fail type analysis and always
	// yield the type of their symbol (which
	// depends on their definition)
	ta->nodeType(this, this->getSymbol()->getDataType());
}

void IntLitNode::typeAnalysis(TypeAnalysis * ta){
	// IntLits never fail their type analysis and always
	// yield the type INT
	ta->nodeType(this, BasicType::produce(INT));
}

// INCOMPLETE -- HOW DO REPRESENT A STRLITNODE
// void StrLitNode::typeAnalysis(TypeAnalysis * ta){
// 	// IntLits never fail their type analysis and always
// 	// yield the type INT
// 	ta->nodeType(this, BasicType::produce(INT));
// }

void CharLitNode::typeAnalysis(TypeAnalysis * ta){
	// IntLits never fail their type analysis and always
	// yield the type INT
	ta->nodeType(this, BasicType::produce(CHAR));
}

void TrueNode::typeAnalysis(TypeAnalysis * ta){
	ta->nodeType(this, BasicType::produce(BOOL));
}

void FalseNode::typeAnalysis(TypeAnalysis * ta){
	ta->nodeType(this, BasicType::produce(BOOL));
}

void CallExpNode::typeAnalysis(TypeAnalysis * ta){
	
	
	
	// myID->typeAnalysis(ta);
	
	// auto ID = ta->nodeType(myID);

	// // ensure ID is a function
	// if(!ID->asFn()) {
	// 	ta->badCallee(myID->line(), myID->col());
	// 	ta->nodeType(this, ErrorType::produce());
	// }

	// for (auto args : *myArgs) {
	// 	args->typeAnalysis(ta);
	// 	auto arg = ta->nodeType(args);
	// 	if(arg->asError()) {

	// 	}
	// }

	// // ensure formals are OK
	// if()

	
	
	// // INCOMPLETE
	// auto idType = ta->nodeType(myID);
	// if (!idType->asFn())
	// {
	// 	ta->badCallee(myID->line(), myID->col());
	// 	ta->nodeType(this, ErrorType::produce());
	// }
	// else if (myArgs->size() != idType->asFn().getFormalTypes().size())

	// ta->nodeType(this, BasicType::produce(BOOL));
}

void NegNode::typeAnalysis(TypeAnalysis *ta){
  myExp->typeAnalysis(ta);
  if (ta->nodeType(myExp) == BasicType::produce(INT)){
    ta->nodeType(this, BasicType::produce(INT));
  } else {
    ta->badMathOpd(myExp->line(), myExp->col());
    ta->nodeType(this, ErrorType::produce());
  }
}

void NotNode::typeAnalysis(TypeAnalysis *ta){
	
	// void -- cant do this
	// int -- cant do this
	// char -- cant do this
	// bool

	if (ta->nodeType(myExp) == BasicType::produce(BOOL)) {
    	ta->nodeType(this, BasicType::produce(BOOL));
	}
	else {
		// not an int or bool .. so we cant do the opposite of it
		ta->badLogicOpd(myExp->line(), myExp->col());
		ta->nodeType(this, ErrorType::produce());
	}

}

void PostIncStmtNode::typeAnalysis(TypeAnalysis *ta){
	
	// void -- cant do this
	// int 
	// char -- cant do this
	// bool -- cant do this

	if (ta->nodeType(myLVal) == BasicType::produce(INT)) {
    	ta->nodeType(this, BasicType::produce(INT));
	}
	else {
		// not an int .. so we cant ++ it
		ta->badMathOpd(myLVal->line(), myLVal->col());
		ta->nodeType(this, ErrorType::produce());
	}

}

void PostDecStmtNode::typeAnalysis(TypeAnalysis *ta){
	
	// void -- cant do this
	// int 
	// char -- cant do this
	// bool -- cant do this

	if (ta->nodeType(myLVal) == BasicType::produce(INT)) {
    	ta->nodeType(this, BasicType::produce(INT));
	}
	else {
		// not an int .. so we cant -- it
		ta->badMathOpd(myLVal->line(), myLVal->col());
		ta->nodeType(this, ErrorType::produce());
	}

}

void BinaryExpNode::typeAnalysis(TypeAnalysis *ta){ // IS THIS CORRECT?
	
	// void -- cant do this
	// int 
	// char -- cant do this
	// bool -- cant do this

	// FOR AN OPERATOR
	// 1. EACH ONE HAS A LEFT AND RIGHT OPERAND
		// DO TYPE ANALAYSIS ON THE OPERAND. IT MIGHT BE A BAD TYPE IN AND OF ITSELF
		// ASSUMING THAT OPERATOR ISN'T BAD, MAKE SURE THAT IT IS OF TYPE INT
		// THEN DO THE RIGHT ONE
		// THEN RETURN TYPE INT?

	// possible errors:
		/*
			1. either side is an error type, so we have a bad left or right operand
			2. void < void, int < bool, char < int
			3. NOTHING ELSE?
				// specify the eror
		*/


	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if (lType->asError())
	{
		ta->badRelOpd(myExp1->line(), myExp1->col()); // yeah, ltype doesn't have a col and line. all the nodes in ast know where they are
		ta->nodeType(this, ErrorType::produce());
	}
	else if (rType->asError())
	{
		ta->badRelOpd(myExp2->line(), myExp2->col());
		ta->nodeType(this, ErrorType::produce());
	}
	else if (lType != rType)
	{
		ta->nodeType(this, ErrorType::produce());
	}
	else
	{
		ta->nodeType(this, lType);
	}
}

void ToConsoleStmtNode::typeAnalysis(TypeAnalysis *ta){
	mySrc->typeAnalysis(ta);
	auto srcType = ta->nodeType(mySrc);
	if (srcType->asFn())
	{
		ta->badToConsole(mySrc->line(), mySrc->col());
		ta->nodeType(this, ErrorType::produce());
	}
	else if (srcType == BasicType::produce(VOID))
	{
		ta->badWriteVoid(mySrc->line(), mySrc->col());
		ta->nodeType(this, ErrorType::produce());
	}
	else
	{
		ta->nodeType(this, srcType);
	}
}

void FromConsoleStmtNode::typeAnalysis(TypeAnalysis *ta){
	myDst->typeAnalysis(ta);
	auto dstType = ta->nodeType(myDst);
	if (dstType->asFn())
	{
		ta->badFromConsole(myDst->line(), myDst->col());
		ta->nodeType(this, ErrorType::produce());
	}
	else
	{
		ta->nodeType(this, dstType);
	}
}

void CallStmtNode::typeAnalysis(TypeAnalysis *ta){
	
}

}
