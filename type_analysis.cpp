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

	for (auto body : *myBody) {
		body->typeAnalysis(ta);
	}
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
	myDst->typeAnalysis(ta);
	mySrc->typeAnalysis(ta);

	const DataType * tgtType = ta->nodeType(myDst);
	const DataType * srcType = ta->nodeType(mySrc);


	if (tgtType->asError() || srcType->asError()) {
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	else if (tgtType->asFn()) { // the dst is a function
		ta->badAssignOpd(myDst->line(), myDst->col());
		if(srcType->asFn()) { // the src is a function, too
			ta->badAssignOpd(mySrc->line(), mySrc->col());
		}
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	else if (tgtType == srcType) { // same type
		if (!(tgtType->validVarType())) { // void == void
			ta->badAssignOpr(myDst->line(), myDst->col());
			ta->badAssignOpr(mySrc->line(), mySrc->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
		ta->nodeType(this, tgtType);
		return;
	}
	else { // bool = int
		ta->badAssignOpr(mySrc->line(), mySrc->col());
		ta->nodeType(this, ErrorType::produce());
		return;
	}
}

void VarDeclNode::typeAnalysis(TypeAnalysis * ta){
	// VarDecls always pass type analysis, since they 
	// are never used in an expression. You may choose
	// to type them void (like this), as discussed in class
	ta->nodeType(this, BasicType::produce(VOID));
}

void FormalDeclNode::typeAnalysis(TypeAnalysis * ta){
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

void CharLitNode::typeAnalysis(TypeAnalysis * ta){
	// IntLits never fail their type analysis and always
	// yield the type CHAR
	ta->nodeType(this, BasicType::produce(CHAR));
}

void TrueNode::typeAnalysis(TypeAnalysis * ta){
	ta->nodeType(this, BasicType::produce(BOOL));
}

void FalseNode::typeAnalysis(TypeAnalysis * ta){
	ta->nodeType(this, BasicType::produce(BOOL));
}

void CallExpNode::typeAnalysis(TypeAnalysis * ta){
	
	auto myFn = myID->getSymbol()->getDataType()->asFn();
	if (myFn == nullptr){
		ta->badCallee(myID->line(), myID->col());
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	auto myFrmls = myFn->getFormalTypes();
	if (myArgs->size() != myFrmls->size()){
		ta->badCallee(myID->line(), myID->col());
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	
	for (auto arg : *myArgs){
		if (ta->nodeType(arg) != myFrmls->front()){
			ta->badCallee(arg->line(), arg->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
		myArgs->pop_front();
	}
	ta->setCurrentFnType(myFn);
	ta->nodeType(this, myFn->getReturnType());
}

void NegNode::typeAnalysis(TypeAnalysis *ta){ 
  myExp->typeAnalysis(ta);
  auto exp = ta->nodeType(myExp);
  if (exp->isInt()){
    ta->nodeType(this, exp);
  } else {
    ta->badMathOpd(myExp->line(), myExp->col());
    ta->nodeType(this, ErrorType::produce());
  }
}

void NotNode::typeAnalysis(TypeAnalysis *ta){
	myExp->typeAnalysis(ta);
  	auto exp = ta->nodeType(myExp);	
	if (exp->isBool()) {
    	ta->nodeType(this, exp);
	}
	else {
		ta->badLogicOpd(myExp->line(), myExp->col());
		ta->nodeType(this, ErrorType::produce());
	}
}

void PostIncStmtNode::typeAnalysis(TypeAnalysis *ta){ // CHECK IF A FUNCTION
	myLVal->typeAnalysis(ta);
	auto lval = ta->nodeType(myLVal);
	if (lval->isInt()) {
    	ta->nodeType(this, lval);
	}
	else {
		ta->badMathOpd(myLVal->line(), myLVal->col());
		ta->nodeType(this, ErrorType::produce());
	}
}

void PostDecStmtNode::typeAnalysis(TypeAnalysis *ta){ // CHECK IF A FUNCTION
	myLVal->typeAnalysis(ta);
	auto lval = ta->nodeType(myLVal);
	if (lval->isInt()) {
    	ta->nodeType(this, lval);
	}
	else {
		ta->badMathOpd(myLVal->line(), myLVal->col());
		ta->nodeType(this, ErrorType::produce());
	}
}

void BinaryExpNode::typeAnalysis(TypeAnalysis *ta){ // this only covers == and !=, funcs are allowed
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if (lType->asError() || rType->asError())
	{
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	else if (lType == rType) {
		if((lType->isInt()) || (lType->isBool()) || (lType->isChar())) {
			ta->nodeType(this, BasicType::produce(BOOL));
		}
		else {
			ta->badEqOpd(myExp1->line(), myExp1->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
		if((rType->isInt()) || (rType->isBool()) || (rType->isChar())) {
			ta->nodeType(this, BasicType::produce(BOOL));
		}
		else {
			ta->badEqOpd(myExp2->line(), myExp2->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	else { // not the same type, so right side is the problem
		ta->badEqOpr(myExp2->line(), myExp2->col());
		ta->nodeType(this, ErrorType::produce());
		return;
	}
}

void LessNode::typeAnalysis(TypeAnalysis *ta) {
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if(lType->isInt()) {
		if(rType->isInt()) {
			ta->nodeType(this, BasicType::produce(BOOL));
			return;
		}
		else {
			ta->badRelOpd(myExp2->line(), myExp2->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	else {
		ta->badRelOpd(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
		if(!(rType->isBool())) {
			ta->badLogicOpd(myExp2->line(), myExp2->col());
			return;
		}
		return;
	}
}

void LessEqNode::typeAnalysis(TypeAnalysis *ta) {
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if(lType->isInt()) {
		if(rType->isInt()) {
			ta->nodeType(this, BasicType::produce(BOOL));
			return;
		}
		else {
			ta->badRelOpd(myExp2->line(), myExp2->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	else {
		ta->badRelOpd(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
		if(!(rType->isBool())) {
			ta->badLogicOpd(myExp2->line(), myExp2->col());
			return;
		}
		return;
	}
}

void GreaterNode::typeAnalysis(TypeAnalysis *ta) {
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if(lType->isInt()) {
		if(rType->isInt()) {
			ta->nodeType(this, BasicType::produce(BOOL));
			return;
		}
		else {
			ta->badRelOpd(myExp2->line(), myExp2->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	else {
		ta->badRelOpd(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
		if(!(rType->isBool())) {
			ta->badLogicOpd(myExp2->line(), myExp2->col());
			return;
		}
		return;
	}
}

void GreaterEqNode::typeAnalysis(TypeAnalysis *ta) {
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if(lType->isInt()) {
		if(rType->isInt()) {
			ta->nodeType(this, BasicType::produce(BOOL));
			return;
		}
		else {
			ta->badRelOpd(myExp2->line(), myExp2->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	else {
		ta->badRelOpd(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
		if(!(rType->isBool())) {
			ta->badLogicOpd(myExp2->line(), myExp2->col());
			return;
		}
		return;
	}
}

void AndNode::typeAnalysis(TypeAnalysis *ta) {
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if(lType->isBool()) {
		if(rType->isBool()) {
			ta->nodeType(this, BasicType::produce(BOOL));
			return;
		}
		else {
			ta->badLogicOpd(myExp2->line(), myExp2->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	else {
		ta->badRelOpd(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
		if(!(rType->isBool())) {
			ta->badLogicOpd(myExp2->line(), myExp2->col());
			return;
		}
		return;
	}
}

void OrNode::typeAnalysis(TypeAnalysis *ta) {
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if(lType->isBool()) {
		if(rType->isBool()) {
			ta->nodeType(this, BasicType::produce(BOOL));
			return;
		}
		else {
			ta->badLogicOpd(myExp2->line(), myExp2->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	else {
		ta->badRelOpd(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
		if(!(rType->isBool())) {
			ta->badLogicOpd(myExp2->line(), myExp2->col());
			return;
		}
		return;
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
	myCallExp->typeAnalysis(ta);
	ta->nodeType(this, BasicType::produce(VOID));
}

void StmtNode::typeAnalysis(TypeAnalysis *ta){
	TODO("override me??");
}

void WhileStmtNode::typeAnalysis(TypeAnalysis *ta){
	myCond->typeAnalysis(ta);
	if (!ta->nodeType(myCond)->isBool())
	{
		ta->badWhileCond(myCond->line(), myCond->col());
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	for (auto stmt : *myBody){
		stmt->typeAnalysis(ta);
	}
	ta->nodeType(this, BasicType::produce(VOID));
}

void IfStmtNode::typeAnalysis(TypeAnalysis *ta){
	myCond->typeAnalysis(ta);
	if (!ta->nodeType(myCond)->isBool()){
		ta->badIfCond(myCond->line(), myCond->col());
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	for (auto stmt : *myBody){
		stmt->typeAnalysis(ta);
	}
	ta->nodeType(this, BasicType::produce(VOID));	
}

void IfElseStmtNode::typeAnalysis(TypeAnalysis *ta) {
	myCond->typeAnalysis(ta);
	if (!ta->nodeType(myCond)->isBool()){
		ta->badIfCond(myCond->line(), myCond->col());
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	for (auto stmt : *myBodyTrue){
		stmt->typeAnalysis(ta);
	}
	for (auto stmt : *myBodyFalse){
		stmt->typeAnalysis(ta);
	}
	ta->nodeType(this, BasicType::produce(VOID));	
}

void PlusNode::typeAnalysis(TypeAnalysis *ta) {
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if(lType->isInt()) {
		if(rType->isInt()) {
			ta->nodeType(this, BasicType::produce(INT)); // int + int , return int
			return;
		}
		else { // int + bool , return error
			ta->badMathOpr(myExp2->line(), myExp2->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	else {  // bool + ?
		ta->badMathOpr(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
		if(!(rType->isInt())) { // bool + char , return error
			ta->badMathOpr(myExp2->line(), myExp2->col());
			return;
		}
		return; // bool + int , return error	
	}
}

void MinusNode::typeAnalysis(TypeAnalysis *ta) {
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if(lType->isInt()) {
		if(rType->isInt()) {
			ta->nodeType(this, BasicType::produce(INT)); 
			return;
		}
		else { 
			ta->badMathOpr(myExp2->line(), myExp2->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	else { 
		ta->badMathOpr(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
		if(!(rType->isInt())) { 
			ta->badMathOpr(myExp2->line(), myExp2->col());
			return;
		}
		return;
	}
}

void DivideNode::typeAnalysis(TypeAnalysis *ta) {
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if(lType->isInt()) {
		if(rType->isInt()) {
			ta->nodeType(this, BasicType::produce(INT)); 
			return;
		}
		else { 
			ta->badMathOpr(myExp2->line(), myExp2->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	else { 
		ta->badMathOpr(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
		if(!(rType->isInt())) { 
			ta->badMathOpr(myExp2->line(), myExp2->col());
			return;
		}
		return;
	}
}

void TimesNode::typeAnalysis(TypeAnalysis *ta) {
	myExp1->typeAnalysis(ta);
	myExp2->typeAnalysis(ta);

	auto lType = ta->nodeType(myExp1); 
	auto rType = ta->nodeType(myExp2);

	if(lType->isInt()) {
		if(rType->isInt()) {
			ta->nodeType(this, BasicType::produce(INT)); 
			return;
		}
		else { 
			ta->badMathOpr(myExp2->line(), myExp2->col());
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	else { 
		ta->badMathOpr(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
		if(!(rType->isInt())) { 
			ta->badMathOpr(myExp2->line(), myExp2->col());
			return;
		}
		return;
	}
}

void EqualsNode::typeAnalysis(TypeAnalysis *ta) {
	auto lType = ta->nodeType(myExp1);
	auto rType = ta->nodeType(myExp2);
	if (lType != rType){
		ta->badEqOpd(myExp1->line(), myExp1->col());
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	else{
		ta->nodeType(this, BasicType::produce(BOOL));
	}
}

void ReturnStmtNode::typeAnalysis(TypeAnalysis *ta) {
	auto fnType = ta->getCurrentFnType();
	if (myExp == nullptr){
		if (fnType->getReturnType()->isVoid()){
			ta->nodeType(this, BasicType::produce(VOID));
			return;
		}
		else{
			ta->badRetValue(0, 0);
			ta->nodeType(this, ErrorType::produce());
			return;
		}
	}
	myExp->typeAnalysis(ta);
	auto myType = ta->nodeType(myExp);
	if (fnType->getReturnType()->isVoid()){
		ta->badRetValue(myExp->line(), myExp->col());
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	if (fnType->getReturnType() != myType){
		ta->badRetValue(myExp->line(), myExp->col());
		ta->nodeType(this, ErrorType::produce());
		return;
	}
	ta->nodeType(this, myType);
}

} // end big thing
