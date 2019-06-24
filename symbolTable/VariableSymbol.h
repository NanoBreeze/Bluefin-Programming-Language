#pragma once

#include <utility>
#include <memory>
#include <iostream>
#include "Symbol.h"

namespace bluefin {

	using std::string;
	using std::shared_ptr;
	using std::move;

	class VariableSymbol : public Symbol {

	public:
		VariableSymbol(const string& name, shared_ptr<Type> type) :
			Symbol(name, move(type)) {}

		inline string getCategoryName() const override { return "var"; }
	};
}
