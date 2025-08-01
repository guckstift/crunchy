#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "crunchy.h"

#define error(...) error_at(cur_token, __VA_ARGS__)

Block *p_block();
Expr *p_expr();

static Token *cur_token = 0;
static Block *cur_block = 0;
static int64_t next_block_id = 0;

int declare(Stmt *decl)
{
	return declare_in(decl, cur_block);
}

int match(Kind kind)
{
	return cur_token->kind == kind;
}

Token *eat(Kind kind)
{
	if(cur_token->kind == kind) return cur_token ++;
	return 0;
}

Token *expect(Kind kind, char *error_msg)
{
	Token *token = eat(kind);
	if(!token) error(error_msg);
	return token;
}

Type *p_primtype()
{
	if(eat(KW_int)) return new_type(TY_INT);
	else if(eat(KW_bool)) return new_type(TY_BOOL);
	else if(eat(KW_string)) return new_type(TY_STRING);
	else if(eat(KW_function)) return new_type(TY_FUNC);
	return 0;
}

Type *p_type()
{
	Type *type = p_primtype();
	if(!type) return 0;

	while(eat(PT_LBRACK)) {
		expect(PT_RBRACK, "expected ] after [ to denote an array type");
		Type *array_type = new_type(TY_ARRAY);
		array_type->subtype = type;
		type = array_type;
	}

	return type;
}

Expr *p_array()
{
	Token *start = eat(PT_LBRACK);
	if(!start) return 0;
	Expr *first_item = 0;
	Expr *last_item = 0;
	int64_t length = 0;

	while(1) {
		Expr *item = p_expr();
		if(!item) break;
		length ++;
		if(last_item) last_item->next = item;
		else first_item = item;
		last_item = item;
		if(!eat(PT_COMMA)) break;
	}

	expect(PT_RBRACK, "expected ] or ,");
	Expr *array = new_expr(EX_ARRAY, start, 0);
	array->items = first_item;
	array->length = length;
	return array;
}

Expr *p_atom()
{
	Token *literal = 0;
	Expr *expr = 0;

	if(literal = eat(TK_INT)) {
		expr = new_expr(EX_INT, literal, 0);
		expr->ival = literal->ival;
	}
	else if(literal = eat(KW_true)) {
		expr = new_expr(EX_BOOL, literal, 0);
		expr->ival = 1;
	}
	else if(literal = eat(KW_false)) {
		expr = new_expr(EX_BOOL, literal, 0);
		expr->ival = 0;
	}
	else if(literal = eat(TK_IDENT)) {
		expr = new_expr(EX_VAR, literal, 1);
		expr->ident = literal;
	}
	else if(literal = eat(TK_STRING)) {
		expr = new_expr(EX_STRING, literal, 1);
		expr->chars = literal->chars;
		expr->length = literal->str_length;
	}
	else if(match(PT_LBRACK)) {
		expr = p_array();
	}

	return expr;
}

Expr *p_call()
{
	Expr *callee = p_atom();
	if(!callee) return 0;

	while(eat(PT_LPAREN)) {
		expect(PT_RPAREN, "expected ) after (");
		Expr *call = new_expr(EX_CALL, callee->start, 0);
		call->callee = callee;
		callee = call;
	}

	return callee;
}

Expr *p_binop()
{
	Expr *left = p_call();
	if(!left) return 0;

	while(1) {
		Token *op = eat(PT_PLUS);
		if(!op) return left;
		Expr *right = p_call();
		if(!right) error("expected right side expression after +");
		Expr *binop = new_expr(EX_BINOP, left->start, 0);
		binop->left = left;
		binop->right = right;
		binop->op = op;
		left = binop;
	}
}

Expr *p_expr()
{
	return p_binop();
}

Stmt *p_vardecl()
{
	Token *start = cur_token;
	if(!eat(KW_var)) return 0;
	Token *ident = expect(TK_IDENT, "missing variable name after var keyword");
	Type *type = 0;
	Expr *init = 0;

	if(eat(PT_COLON)) {
		type = p_type();
		if(!type) error("missing type specification after colon");
	}

	if(eat(PT_EQUALS)) {
		init = p_expr();
		if(!init) error("missing expression after equals");
	}

	if(!type && !init)
		error("a variable declaration must have at least either a type specification or an initializer expression");

	Stmt *stmt = new_stmt(ST_VARDECL, cur_block, start, cur_token);
	stmt->ident = ident;
	stmt->type = type;
	stmt->init = init;
	if(!declare(stmt)) error_at(ident, "name %n is already declared", ident);
	expect(PT_SEMICOLON, "missing semicolon after variable declaration");
	stmt->end = cur_token;
	return stmt;
}

Stmt *p_funcdecl()
{
	Token *start = cur_token;
	if(!eat(KW_function)) return 0;
	if(cur_block->parent) error("functions must be declared at the top level");
	Token *ident = expect(TK_IDENT, "missing function name after function keyword");
	expect(PT_LPAREN, "missing '(' after function name");
	expect(PT_RPAREN, "missing ')' after '('");
	expect(PT_LCURLY, "missing '{' after function head");
	Block *body = p_block();
	expect(PT_RCURLY, "missing '}' after function body");
	Stmt *stmt = new_stmt(ST_FUNCDECL, cur_block, start, cur_token);
	stmt->ident = ident;
	stmt->body = body;
	if(!declare(stmt)) error_at(ident, "name %n is already declared", ident);
	return stmt;
}

Stmt *p_print()
{
	Token *start = cur_token;
	if(!eat(KW_print)) return 0;
	Expr *value = p_expr();
	if(!value) error("missing expression to print");
	Stmt *stmt = new_stmt(ST_PRINT, cur_block, start, cur_token);
	stmt->value = value;
	expect(PT_SEMICOLON, "missing semicolon after print statement");
	stmt->end = cur_token;
	return stmt;
}

Stmt *p_if()
{
	Token *start = cur_token;
	if(!eat(KW_if)) return 0;
	Expr *cond = p_expr();
	if(!cond) error("missing condition after if keyword");
	expect(PT_LCURLY, "expected '{' after if-condition");
	Block *body = p_block();
	expect(PT_RCURLY, "expected '}' after if-body");
	Block *else_body = 0;

	if(eat(KW_else)) {
		expect(PT_LCURLY, "expected '{' after else keyword");
		else_body = p_block();
		expect(PT_RCURLY, "expected '}' after else-body");
	}

	Stmt *stmt = new_stmt(ST_IF, cur_block, start, cur_token);
	stmt->cond = cond;
	stmt->body = body;
	stmt->else_body = else_body;
	return stmt;
}

Stmt *p_assign_or_call()
{
	Expr *target = p_expr();
	if(!target) return 0;

	if(eat(PT_EQUALS)) {
		Expr *value = p_expr();
		if(!value) error("expected assignment value after '='");
		expect(PT_SEMICOLON, "missing semicolon after variable declaration");
		Stmt *stmt = new_stmt(ST_ASSIGN, cur_block, target->start, cur_token);
		stmt->target = target;
		stmt->value = value;
		return stmt;
	}

	if(target->kind != EX_CALL)
		error_at(target->start, "this is not an assignment nor call statement");

	expect(PT_SEMICOLON, "missing semicolon after call statement");
	Stmt *stmt = new_stmt(ST_CALL, cur_block, target->start, cur_token);
	stmt->call = target;
	return stmt;
}

Stmt *p_stmt()
{
	Stmt *stmt = 0;
	(stmt = p_vardecl()) ||
	(stmt = p_funcdecl()) ||
	(stmt = p_print()) ||
	(stmt = p_if()) ||
	(stmt = p_assign_or_call()) ;
	return stmt;
}

Block *p_block()
{
	Block *old_block = cur_block;
	Block *block = calloc(1, sizeof(Block));
	Stmt *first = 0;
	Stmt *last = 0;
	cur_block = block;
	cur_block->parent = old_block;
	cur_block->id = next_block_id;
	next_block_id ++;

	while(1) {
		Stmt *stmt = p_stmt();
		if(!stmt) break;

		if(!first) {
			cur_block->stmts = stmt;
			first = stmt;
			last = stmt;
		}
		else {
			last->next = stmt;
			last = stmt;
		}
	}

	cur_block = old_block;
	return block;
}

Block *parse(Token *tokens)
{
	cur_token = tokens;
	eat(TK_BOF);
	Block *main_block = p_block();
	expect(TK_EOF, "invalid statement");
	return main_block;
}