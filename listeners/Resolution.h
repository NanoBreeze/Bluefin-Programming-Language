#pragma once

#include "antlr4-runtime.h"
#include <memory>
#include <map>
#include <stack>
#include <vector>
#include "../symbolTable/EventObserver.h"
#include "../generated/bluefin/bluefinBaseListener.h"
#include "../symbolTable/SymbolTable.h"
#include "../symbolTable/StructSymbol.h"
#include "../symbolTable/Scope.h"

namespace bluefin {

	using std::shared_ptr;
	using std::map;
	using std::stack;
	using std::pair;
	using std::vector;
	using antlr4::tree::ParseTree;

	class EventObserver;

	/*
	Resolves symbol references. To do so we need the appropriate scope associated with the contexts. 
	This should be passed from the Declaration phase. Resolving symbols after all have been declared 
	allows forward reference. To restrict forward references (only to within classes), we compare the 
	position of the context to resolve with the position of the context of the actual resolved symbol 
	(assumes it is resolved). Illegal is current context occurs before the other one.
	*/
	class Resolution : public bluefinBaseListener
	{
	public:
		// For testing, we'll pass in an adapter of a symbol table
		// TODO: find some way to decouple testing of output from Resolution
		Resolution(map<ParseTree*, shared_ptr<Scope>> scopes, SymbolTable& symTab) : 
			scopes{ scopes },  symbolTable{symTab}
		{}

		void enterVarDecl(bluefinParser::VarDeclContext * ctx) override;
		void enterParam(bluefinParser::ParamContext * ctx) override;
		void enterFuncDef(bluefinParser::FuncDefContext * ctx) override;
		void enterPrimaryId(bluefinParser::PrimaryIdContext*) override;
		void exitMemberAccess(bluefinParser::MemberAccessContext*) override;
		void enterFuncCall(bluefinParser::FuncCallContext*) override;
		void exitMethodCall(bluefinParser::MethodCallContext*) override;

		void attachEventObserver(shared_ptr<EventObserver>);
		void detachEventObserver(shared_ptr<EventObserver>); // is this even called? If arg not found, no error would be thrown

	private:
		map<ParseTree*, shared_ptr<Scope>> scopes;
		pair<shared_ptr<Symbol>, shared_ptr<Scope>> resolve(const string name, shared_ptr<Scope> startScope);

		SymbolTable& symbolTable;
		/* 
		The purpose of this stack is to enable resolution of struct members. Since each 
		listener's return type is void, there's no implicit way for the child of a node 
		to return a value to its parent. We can't use the call stack so we create an 
		explicit stack. An example of the tree for a.b: 
										  stmtExpr
							expr:memberAccess		;
						expr:primaryId	.	b
								a

		1. We want to resolve 'a' as a StructSymbol
		2. Resolve 'b' as a member of 'a'
		We pass the resolution of 'a' to memberAccess so it can resolve 'b' from 'a' with a stack

		If primaryId's category is a StructSymbol, push onto stack
		memberAccess must pop from the stack to get the first component, which, ofc, we expect to be a StructSymbol.
		If a.b is a struct, then we must should it onto the stack
		TODO: To Verify. At the end of a stmExpr, we expect that the stack is empty (# of uses = # of resolved)
		*/

		stack<shared_ptr<StructSymbol>> structSymbolStack;
		vector<shared_ptr<EventObserver>> eventObservers;

		void broadcastEvent(SuccessEvent, shared_ptr<Symbol>, shared_ptr<StructSymbol> structSym = nullptr);
		void broadcastEvent(ErrorEvent, string, shared_ptr<StructSymbol> structSym=nullptr);

		pair<shared_ptr<Symbol>, shared_ptr<Scope>> resolveImpl(const string name, shared_ptr<Scope> startScope);	

	};
}