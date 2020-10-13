#pragma once

#include "antlr4-runtime.h"
#include <memory>
#include <unordered_map>
#include "Scope.h"
#include "../TestBluefin/utils.h"

namespace bluefin {

	using std::shared_ptr;
	using std::unique_ptr;
	using std::make_shared;
	using std::unordered_map;
	using antlr4::tree::ParseTree;

	class StructSymbol;
	class EventObserver;


	class SymbolTable
	{
	public:
		SymbolTable(); 
		
		void enterScope(const string scopeName="");

		void setCurrentScope(shared_ptr<Scope>);
	
		void exitScope();

		void declare(shared_ptr<Symbol> symbol, ParseTree* context=nullptr);

		/*
		Find the name in the curr scope, if not, find in its 
		parent scope, and continue bubbling upwards. 
		If not found, return nullptr
		*/
		shared_ptr<Symbol> resolve(const string name, const shared_ptr<Scope> scope) const;

		/* 
		Why might somebody want the current scope?
		1. Testing
		2. To map the current context with the current scope, passing the symbols for future listener passes
		*/
		inline shared_ptr<Scope> getCurrScope() const { return currScope; }

		shared_ptr<Symbol> getSymbolMatchingType(Type type);

		void saveParseTreeWithCurrentScope(ParseTree*);

		shared_ptr<Scope> getScope(ParseTree*) const;

		shared_ptr<Scope> getScope(shared_ptr<Symbol>) const; // alternatively, we can make each Symbol contain a weak ref to its contianing scope

	private:
		void addUserDefinedType(shared_ptr<StructSymbol>);

		shared_ptr<Scope> currScope; // shared_ptr b/c it can refer to the same scope as a StructSymbol's
		unordered_map<Type, shared_ptr<Symbol>> typeSymbols;

		/* Declaration stores the symbol and scope associated with ParseTree contexts, so that future passes
		(Resolution and type promotion) can access the scope info. Note, to resolve struct member access
		we must store the StructSymbol as a scope (eg, for a.b.c, we must store the StructSymbol for a.b 
		inside the context to later resolve c)
		*/
		struct Context {
			shared_ptr<Scope> scope; // the scope that the Symbol is in
			shared_ptr<Symbol> sym;
		};
		unordered_map<ParseTree*, Context> parseTreeContexts;
	};

}