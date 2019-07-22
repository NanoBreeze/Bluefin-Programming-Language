#include "SymbolFactory.h"
#include <utility>
#include <memory>

using std::move;
using std::unique_ptr;
using std::shared_ptr;

using namespace bluefin;

shared_ptr<Symbol> SymbolFactory::createBuiltinTypeSymbol(Builtin builtin) {
	switch (builtin) {
	case Builtin::BOOL:
		return BuiltinTypeSymbol::BOOL();
	case Builtin::INT:
		return BuiltinTypeSymbol::INT();
	case Builtin::FLOAT:
		return BuiltinTypeSymbol::FLOAT();
	case Builtin::STRING:
		return BuiltinTypeSymbol::STRING();
	case Builtin::VOID:
		return BuiltinTypeSymbol::VOID();
	default:
		throw "not a valid selection";
	}
}

unique_ptr<Symbol> SymbolFactory::createFunctionSymbol(const string& name, shared_ptr<Type> type, size_t tokenIndex) {
	return unique_ptr<Symbol>(new FunctionSymbol(name, type, tokenIndex));
}

unique_ptr<Symbol> SymbolFactory::createStructSymbol(const string& name, shared_ptr<Scope> enclosingScope, shared_ptr<StructSymbol> superClass) {
	return unique_ptr<Symbol>(new StructSymbol(name, enclosingScope, superClass));
}

unique_ptr<Symbol> SymbolFactory::createVariableSymbol(const string& name, shared_ptr<Type> type, size_t tokenIndex) {
	return unique_ptr<Symbol>(new VariableSymbol(name, type, tokenIndex));
}
