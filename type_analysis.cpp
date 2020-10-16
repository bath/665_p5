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
	// Need to check if returnstmt's type matches function's return value, AND need to check if all statements pass typeAnalysis
	// yeah makes sense, then if those pass return as void
	// also do we need to make sure formals pass as well?

	// PROCESS TO RETURN VOID
	// 1. RETURNSTMTTYPE == FN TYPE ... if(myID type == myRetType) 
	// 2. STMTNODE ALL PASS
	// 3. FORMAL DECLS ALL PASS (?)
	// 4. THEN WE RETURN VOID
	// ELSE RETURN AS ERROR ??

	//HINT: you might want to change the signature for
	// typeAnalysis on FnBodyNode to take a second <- there is no FnBodyNode type.
	// argument which is the type of the current 
	// function. This will help you to know at a 
	// return statement whether the return type matches
	// the current function

	//Note: this function may need extra code
	ReturnStmtNode * retStmt;
	for (auto stmt : *myBody){
		stmt->typeAnalysis(ta);
	}
	for (auto decl : *myFormals){
		decl->typeAnalysis(ta);
	}

	myID->typeAnalysis(ta); // do we need to do this?
	//myRetType->typeAnalysis(ta); // do we need to do this?

	auto idType = ta->nodeType(myID);
	auto retType = ta->nodeType(myRetType);

	// i feel like we need to do this similar to assignstmtnode
	if ((idType->asError() || retType->asError()) || (idType != retType)){
		ta->nodeType(this, ); //error case
	} else {
		ta->nodeType(this, BasicType::produce(VOID));
	}

	auto subType = ta->nodeType(myExp);

	// As error returns null if subType is NOT an error type
	// otherwise, it returns the subType itself
	if (subType->asError()){
		ta->nodeType(this, subType);
	} else {
		ta->nodeType(this, BasicType::produce(VOID));
	}

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

void AssignExpNode::typeAnalysis(TypeAnalysis * ta){
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
	if (tgtType == srcType){
		ta->nodeType(this, tgtType);
		return;
	}
	else {
	ta->badAssignOpr(this->line(), this->col());
	ta->nodeType(this, ErrorType::produce());	
	}

	
	//Some functions are already defined for you to
	// report type errors. Note that these functions
	// also tell the typeAnalysis object that the
	// analysis has failed, meaning that main.cpp
	// will print "Type check failed" at the end


	//Note that reporting an error does not set the
	// type of the current node, so setting the node
	// type must be done
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
	// INCOMPLETE
	ta->nodeType(this, BasicType::produce(BOOL));
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

void PlusNode::typeAnalysis(TypeAnalysis *ta){ // IS THIS CORRECT?
	
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

	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	if((ta->nodeType(myExp1) == BasicType::produce(INT)) && (ta->nodeType(myExp2) == BasicType::produce(INT))) {
		ta->nodeType(this, BasicType::produce(INT));
	}
	if(ta->nodeType(myExp1) != BasicType::produce(INT)) {
		ta->badMathOpd(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
	}
	if(ta->nodeType(myExp2) != BasicType::produce(INT)) {
		ta->badMathOpd(myExp2->line(), myExp2->col());
		ta->nodeType(this, ErrorType::produce());
	}
}

}
