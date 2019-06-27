#include "DecorateExprWithTypes.h"

#include <memory>
#include <iostream>

using namespace bluefin;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using std::static_pointer_cast;
using std::move;
using antlr4::tree::ParseTreeProperty;
using std::cerr; 
using std::endl;

//=============== listener methods for expressions
void DecorateExprWithTypes::exitPrimaryInt(bluefinParser::PrimaryIntContext* ctx) {

	typeContexts.emplace(ctx, 
		TypeContext { 
			dynamic_pointer_cast<Type>(shared_ptr<Symbol>(move(symbolFactory.createBuiltinTypeSymbol(BuiltinTypeSymbol::BuiltinType::INT)))), //what the heck is this monstrosity!
		}
	); // what is this monstrosity?
}

void DecorateExprWithTypes::exitPrimaryFloat(bluefinParser::PrimaryFloatContext* ctx) {
	typeContexts.emplace(ctx, 
		TypeContext{ 
			dynamic_pointer_cast<Type>(shared_ptr<Symbol>(move(symbolFactory.createBuiltinTypeSymbol(BuiltinTypeSymbol::BuiltinType::FLOAT)))),
		}
	);
}

void DecorateExprWithTypes::exitPrimaryString(bluefinParser::PrimaryStringContext* ctx) {
	typeContexts.emplace(ctx,
		TypeContext { 
			dynamic_pointer_cast<Type>(shared_ptr<Symbol>(move(symbolFactory.createBuiltinTypeSymbol(BuiltinTypeSymbol::BuiltinType::STRING)))),
		}
	);
}

void DecorateExprWithTypes::exitPrimaryBool(bluefinParser::PrimaryBoolContext* ctx) {
	typeContexts.emplace(ctx,
		TypeContext { 
			dynamic_pointer_cast<Type>(shared_ptr<Symbol>(move(symbolFactory.createBuiltinTypeSymbol(BuiltinTypeSymbol::BuiltinType::BOOL)))),
		}
	);
}

void DecorateExprWithTypes::exitPrimaryId(bluefinParser::PrimaryIdContext* ctx) {
	shared_ptr<Scope> scope = scopeOfPrimaryIds.at(ctx);
	shared_ptr<Symbol> resolvedSym = resolve(ctx->ID()->getText(), scope);

	if (resolvedSym) // maybe the symbol is unresolved?
	{
		typeContexts.emplace(ctx, TypeContext { resolvedSym->getType() });
	}
}

void DecorateExprWithTypes::exitPrimaryParenth(bluefinParser::PrimaryParenthContext* ctx) {
	shared_ptr<Type> subExprType = typeContexts.at(ctx->expr()).getEvalType(); // don't use [] b/c TypeContext doesn't have default ctor
	typeContexts.emplace(ctx, TypeContext{ subExprType });
}

void DecorateExprWithTypes::exitFuncCall(bluefinParser::FuncCallContext* ctx) {
	shared_ptr<Type> subExprType = typeContexts.at(ctx->expr()).getEvalType();
	typeContexts.emplace(ctx, TypeContext { subExprType });
}

void DecorateExprWithTypes::exitUnaryExpr(bluefinParser::UnaryExprContext* ctx) {
	shared_ptr<Type> subExprType = typeContexts.at(ctx->expr()).getEvalType();

	if (ctx->op->getText() == "!") {
		if (subExprType->getTypePossibility() != TP::BOOL) {
			cerr << "Bad unary ! operand type" << endl;
		}
	}
	else { // - (negative)
		if (subExprType->getTypePossibility() != TP::BOOL || subExprType->getTypePossibility() != TP::FLOAT) { // TODO, how about just comparing type instead of TP
			cerr << "Bad unary - operand type" << endl;
		}
	}
	typeContexts.emplace(ctx, TypeContext { subExprType });
}

void DecorateExprWithTypes::exitMultiExpr(bluefinParser::MultiExprContext* ctx) {
	
	TypeContext& leftTypeContext = typeContexts.at(ctx->expr(0));
	TypeContext& rightTypeContext = typeContexts.at(ctx->expr(1));

	if (areArithmeticallyCompatible(leftTypeContext.getEvalType(), rightTypeContext.getEvalType())) {
		leftTypeContext.setPromotionType(
			getPromotionType(leftTypeContext.getEvalType(), rightTypeContext.getEvalType()));
		rightTypeContext.setPromotionType(
			getPromotionType(rightTypeContext.getEvalType(), leftTypeContext.getEvalType()));

		shared_ptr<Type> exprType = getArithmeticExprType(leftTypeContext.getEvalType(), rightTypeContext.getEvalType());
		typeContexts.emplace(ctx, TypeContext { exprType });
	}
	else {
		cerr << "Bad multiExpr operand types" << endl;
	}
}

void DecorateExprWithTypes::exitAddExpr(bluefinParser::AddExprContext* ctx) {
	TypeContext& leftTypeContext = typeContexts.at(ctx->expr(0));
	TypeContext& rightTypeContext = typeContexts.at(ctx->expr(1));

	if (areArithmeticallyCompatible(leftTypeContext.getEvalType(), rightTypeContext.getEvalType())) {
		leftTypeContext.setPromotionType(
			getPromotionType(leftTypeContext.getEvalType(), rightTypeContext.getEvalType()));
		rightTypeContext.setPromotionType(
			getPromotionType(rightTypeContext.getEvalType(), leftTypeContext.getEvalType()));

		shared_ptr<Type> exprType = getArithmeticExprType(leftTypeContext.getEvalType(), rightTypeContext.getEvalType());
		typeContexts.emplace(ctx, TypeContext { exprType });
	}
	else {
		cerr << "Bad addExpr operand types" << endl;
	}
}

void DecorateExprWithTypes::exitRelExpr(bluefinParser::RelExprContext* ctx) {
	typeContexts.emplace(ctx,
		TypeContext { 
			dynamic_pointer_cast<Type>(shared_ptr<Symbol>(move(symbolFactory.createBuiltinTypeSymbol(BuiltinTypeSymbol::BuiltinType::BOOL)))),
		}
	);

	// may need to promote children's type. eg, float > int
	TypeContext& leftTypeContext = typeContexts.at(ctx->expr(0));
	TypeContext& rightTypeContext = typeContexts.at(ctx->expr(1));

	if (areArithmeticallyCompatible(leftTypeContext.getEvalType(), rightTypeContext.getEvalType())) {
		leftTypeContext.setPromotionType(
			getPromotionType(leftTypeContext.getEvalType(), rightTypeContext.getEvalType()));
		rightTypeContext.setPromotionType(
			getPromotionType(rightTypeContext.getEvalType(), leftTypeContext.getEvalType()));
	}
	else {
		cerr << "Bad relExpr operand types" << endl;
	}
}

void DecorateExprWithTypes::exitEqualityExpr(bluefinParser::EqualityExprContext* ctx) {
	typeContexts.emplace(ctx,
		TypeContext { 
			dynamic_pointer_cast<Type>(shared_ptr<Symbol>(move(symbolFactory.createBuiltinTypeSymbol(BuiltinTypeSymbol::BuiltinType::BOOL)))),
		}
	);

	// may need to convert children's type. eg, float > int
	TypeContext& leftTypeContext = typeContexts.at(ctx->expr(0));
	TypeContext& rightTypeContext = typeContexts.at(ctx->expr(1));

	// if lhs and rhs are both the same type or can be promoted to match each other (aka, arithmetically compatible), then they are valid semantics
	if (areSameTypes(leftTypeContext.getEvalType(), rightTypeContext.getEvalType()) ||
		areArithmeticallyCompatible(leftTypeContext.getEvalType(), rightTypeContext.getEvalType())) {

		leftTypeContext.setPromotionType(
			getPromotionType(leftTypeContext.getEvalType(), rightTypeContext.getEvalType()));
		rightTypeContext.setPromotionType(
			getPromotionType(rightTypeContext.getEvalType(), leftTypeContext.getEvalType()));
	}
	else {
		cerr << "Bad equalityExpr operand types" << endl;
	}

}

void DecorateExprWithTypes::exitLogicalANDExpr(bluefinParser::LogicalANDExprContext* ctx) {
	typeContexts.emplace(ctx,
		TypeContext { 
			dynamic_pointer_cast<Type>(shared_ptr<Symbol>(move(symbolFactory.createBuiltinTypeSymbol(BuiltinTypeSymbol::BuiltinType::BOOL)))),
		}
	);

	TypeContext& leftTypeContext = typeContexts.at(ctx->expr(0));
	TypeContext& rightTypeContext = typeContexts.at(ctx->expr(1));

	if (areBothBoolType(leftTypeContext.getEvalType(), rightTypeContext.getEvalType())) {
		//
	}
	else {
		cerr << "Bad logicalANDExpr not good operand types" << endl;
	}
}

void DecorateExprWithTypes::exitLogicalORExpr(bluefinParser::LogicalORExprContext* ctx) {
	typeContexts.emplace(ctx,
		TypeContext { 
			dynamic_pointer_cast<Type>(shared_ptr<Symbol>(move(symbolFactory.createBuiltinTypeSymbol(BuiltinTypeSymbol::BuiltinType::BOOL)))),
		}
	);

	TypeContext& leftTypeContext = typeContexts.at(ctx->expr(0));
	TypeContext& rightTypeContext = typeContexts.at(ctx->expr(1));

	if (areBothBoolType(leftTypeContext.getEvalType(), rightTypeContext.getEvalType())) {
		//
	}
	else {
		cerr << "Bad logicalORExpr not good operand types" << endl;
	}

}

void DecorateExprWithTypes::exitSimpleAssignExpr(bluefinParser::SimpleAssignExprContext* ctx) {

	TypeContext& leftTypeContext = typeContexts.at(ctx->expr(0));
	TypeContext& rightTypeContext = typeContexts.at(ctx->expr(1));

	if (isAssignmentCompatible(leftTypeContext.getEvalType(), rightTypeContext.getEvalType())) {
		rightTypeContext.setPromotionType(
			getPromotionType(rightTypeContext.getEvalType(), leftTypeContext.getEvalType()));

		shared_ptr<Type> exprType = getArithmeticExprType(leftTypeContext.getEvalType(), rightTypeContext.getEvalType());
		typeContexts.emplace(ctx, TypeContext { exprType });
	}
	else {
		cerr << "Bad assignment expr" << endl;
	}
}

void DecorateExprWithTypes::exitMemberAccess(bluefinParser::MemberAccessContext* ctx) {
	// put type of member. Eg, first.a could have type int
	TypeContext& typeContext = typeContexts.at(ctx->expr());

	shared_ptr<StructSymbol> structType = static_pointer_cast<StructSymbol>(typeContext.getEvalType());
	shared_ptr<Type> memberType = structType->resolveMember(ctx->ID()->getText())->getType();
	typeContexts.emplace(ctx, TypeContext { memberType });
}

// It doesn't make sense for a var declaration to have a type (the varsymbol can though)
// b/c it would have no meaning. 
// However, the rhs variable may need to be promoted
// eg, float a = 56
void DecorateExprWithTypes::exitVarDecl(bluefinParser::VarDeclContext* ctx) {
	if (ctx->expr()) {

		// find type of the variable
		shared_ptr<Scope> scope = scopeOfPrimaryIds.at(ctx);
		shared_ptr<Symbol> resolvedSym = resolve(ctx->ID()->getText(), scope);

		TypeContext& rightTypeContext = typeContexts.at(ctx->expr());

		if (isAssignmentCompatible(resolvedSym->getType(), rightTypeContext.getEvalType())) {
			
			// promotion is only possible for arithmetically compatible types.
			// TODO: I feel this layer of abstraction requires some thinking. What if promotion should be allowed in diff contexts
			if (areArithmeticallyCompatible(resolvedSym->getType(), rightTypeContext.getEvalType())) {
				rightTypeContext.setPromotionType(
					getPromotionType(rightTypeContext.getEvalType(), resolvedSym->getType()));
			}
		}
		else {
			cerr << "Bad varDecl expr" << endl;
		}
	}
}

/* We're assuming a return statement will only occur within a function*/
void DecorateExprWithTypes::exitStmtReturn(bluefinParser::StmtReturnContext* ctx)  { 
	// match expr type with the function's return type

	/* How do we get a stmtReturn's enclosing function?
	if (ctx->expr()) {
		TypeContext& retTypeCtx = typeContexts.at(ctx->expr());

		retTypeCtx.setPromotionType(
			getPromotionType(funcRetType, retTypeCtx.getEvalType());
		)
	}
	*/
}


shared_ptr<Symbol> DecorateExprWithTypes::resolve(const string name, shared_ptr<Scope> startScope) {

	shared_ptr<Scope> scope = startScope;

	do {
		shared_ptr<Symbol> sym = scope->resolve(name);
		if (sym) { return sym; }
	} while (scope = scope->getEnclosingScope());

	throw UnresolvedSymbolException(name);
}

using TP = Type::Possibility;

bool DecorateExprWithTypes::areSameTypes(shared_ptr<Type> left, shared_ptr<Type> right) const {
	return left->getTypePossibility() == right->getTypePossibility(); // TODO: but this doesn't work for two different structs!
}

bool DecorateExprWithTypes::areBothBoolType(shared_ptr<Type> left, shared_ptr<Type> right) const {
	return left->getTypePossibility() == TP::BOOL && right->getTypePossibility() == TP::BOOL;
}

// Arithmetics (+,-,*,/) are allowed only on float and int
bool DecorateExprWithTypes::areArithmeticallyCompatible(shared_ptr<Type> left, shared_ptr <Type> right) const {
	TP leftPossibility = left->getTypePossibility();
	TP rightPossibility = right->getTypePossibility();

	return (leftPossibility == TP::INT || leftPossibility == TP::FLOAT) &&
		(rightPossibility == TP::INT || rightPossibility == TP::FLOAT);
}

// used for simple assignment and for varDecl (eg, int a = 5;)
// Two types can be assignable if they are the same possibility or the rhs can be promoted to the lhs
bool DecorateExprWithTypes::isAssignmentCompatible(shared_ptr<Type> left, shared_ptr<Type> right) const {
	if (left->getTypePossibility() == right->getTypePossibility()) {
		return true;
	}

	// check if rhs can be promoted to lhs. The only promotion in this program is int->float, so then
	// we'll just check if lhs is float and rhs is int
	return left->getTypePossibility() == TP::FLOAT &&
		right->getTypePossibility() == TP::INT;
}

shared_ptr<Type> DecorateExprWithTypes::getArithmeticExprType(shared_ptr<Type> left, shared_ptr<Type> right) {
	return arithmeticExprType.at({ left->getTypePossibility(), right->getTypePossibility() });
}

shared_ptr<Type> DecorateExprWithTypes::getPromotionType(shared_ptr<Type> left,shared_ptr<Type>right) {
	return promotionFromTo.at({ left->getTypePossibility(), right->getTypePossibility() });
}

