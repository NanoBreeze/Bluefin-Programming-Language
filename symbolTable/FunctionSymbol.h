#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include "Symbol.h"

namespace bluefin {

	using std::string;
	using std::shared_ptr;
	using std::move;

	/* TODO: should this contain its own arguments?
	 So I imagine a call to SymbolTable::enterScope will occur on method decalaration.
	 This means that function param will be in the same scope as the local variables.
	 TODO: need to remove declaration scope from Standard
	 Do we want to distinguish between function params (which can contain arguments) as a scope
	 Or just a block scope...hmmmm
	*/
	class FunctionSymbol : public Symbol {
	public:

		FunctionSymbol(const string& name, shared_ptr<Type> type) :
			Symbol(name, move(type))
		{}

		inline string getCategoryName() const override { return "function"; }
	};
}
