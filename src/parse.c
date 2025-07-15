#include <stdlib.h>
#include "crunchy.h"

Token *cur_token = 0;

Token *eat(TokenKind kind)
{
	if(cur_token->kind == kind) {
		return cur_token ++;
	}

	return 0;
}

Type *p_type()
{
	Token *keyword = 0;
	TypeKind kind = TY_INVALID;

	if(eat(KW_int)) {
		kind = TY_INT;
	}
	else if(eat(KW_bool)) {
		kind = TY_BOOL;
	}
	else {
		return 0;
	}

	Type *type = malloc(sizeof(Type));
	type->kind = kind;
	return type;
}

Expr *p_expr()
{
	Token *literal = 0;
	Expr *expr = 0;

	if(literal = eat(TK_INT)) {
		expr = malloc(sizeof(Expr));
		expr->kind = EX_INT;
		expr->ival = literal->ival;
	}
	else if(literal = eat(KW_true)) {
		expr = malloc(sizeof(Expr));
		expr->kind = EX_BOOL;
		expr->ival = 1;
	}
	else if(literal = eat(KW_false)) {
		expr = malloc(sizeof(Expr));
		expr->kind = EX_BOOL;
		expr->ival = 0;
	}
	else {
		return 0;
	}

	return expr;
}

Stmt *p_stmt()
{
	if(!eat(KW_var)) {
		return 0;
	}

	Token *ident = eat(TK_IDENT);

	if(!ident) {
		error("missing variable name after var keyword");
	}

	Type *type = 0;

	if(eat(PT_COLON)) {
		type = p_type();

		if(!type) {
			error("missing type specification after colon");
		}
	}

	Expr *init = 0;

	if(eat(PT_EQUALS)) {
		init = p_expr();

		if(!init) {
			error("missing expression after equals");
		}
	}

	if(!type && !init) {
		error("a variable declaration must have at least either a type specification or an initializer expression");
	}

	if(!eat(PT_SEMICOLON)) {
		error("missing semicolon after variable declaration");
	}

	Stmt *stmt = malloc(sizeof(Stmt));
	stmt->kind = ST_VARDECL;
	stmt->next = 0;
	stmt->ident = ident;
	stmt->type = type;
	stmt->init = init;
	return stmt;
}

Stmt *parse(Token *tokens)
{
	cur_token = tokens;

	Stmt *first = 0;
	Stmt *last = 0;

	while(1) {
		Stmt *stmt = p_stmt();

		if(!stmt) {
			break;
		}

		if(!first) {
			first = stmt;
			last = stmt;
		}
		else {
			last->next = stmt;
			last = stmt;
		}
	}

	if(!eat(TK_EOF)) {
		error("invalid statement");
	}

	return first;
}