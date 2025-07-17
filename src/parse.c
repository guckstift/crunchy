#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "crunchy.h"

#define error(...) parse_error(__VA_ARGS__)

static char *src_file_start = 0;
static Token *cur_token = 0;
static Block *cur_block = 0;

void parse_error(char *msg)
{
	printf("error: %s\n", msg);

	if(!src_file_start) {
		exit(EXIT_FAILURE);
	}

	printf("%li: ", cur_token->line);
	char *p = cur_token->start;

	while(p > src_file_start && p[-1] != '\n') {
		p --;
	}

	while(*p && *p != '\n') {
		fputc(*p, stdout);
		p ++;
	}

	printf("\n");
	exit(EXIT_FAILURE);
}

Type *new_type(TypeKind kind)
{
	Type *type = calloc(1, sizeof(Type));
	type->kind = kind;
	return type;
}

Expr *new_expr(ExprKind kind, Token *start, uint8_t is_lvalue)
{
	Expr *expr = calloc(1, sizeof(Expr));
	expr->kind = kind;
	expr->start = start;
	expr->is_lvalue = is_lvalue;
	return expr;
}

Stmt *new_stmt(StmtKind kind, Token *start, Token *end)
{
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->kind = kind;
	stmt->next = 0;
	stmt->start = start;
	stmt->end = end;
	return stmt;
}

int declare(Stmt *decl)
{
	for(Stmt *d = cur_block->decls; d; d = d->next_decl) {
		if(d->ident->length == decl->ident->length && memcmp(d->ident->start, decl->ident->start, d->ident->length) == 0) {
			return 0;
		}
	}

	if(cur_block->decls) {
		cur_block->last_decl->next_decl = decl;
		cur_block->last_decl = decl;
	}
	else {
		cur_block->decls = decl;
		cur_block->last_decl = decl;
	}

	return 1;
}

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

	return new_type(kind);
}

Expr *p_expr()
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

	return expr;
}

Stmt *p_vardecl()
{
	Token *start = cur_token;

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

	Stmt *stmt = new_stmt(ST_VARDECL, start, cur_token);
	stmt->ident = ident;
	stmt->type = type;
	stmt->init = init;
	stmt->next_decl = 0;

	if(!declare(stmt)) {
		error("variable is already declared");
	}

	if(!eat(PT_SEMICOLON)) {
		error("missing semicolon after variable declaration");
	}

	return stmt;
}

Stmt *p_assign()
{
	Expr *target = p_expr();

	if(!target) {
		return 0;
	}

	if(!eat(PT_EQUALS)) {
		error("expected '=' after assignment target");
	}

	Expr *value = p_expr();

	if(!value) {
		error("expected assignment value after '='");
	}

	if(!eat(PT_SEMICOLON)) {
		error("missing semicolon after variable declaration");
	}

	Stmt *stmt = new_stmt(ST_ASSIGN, target->start, cur_token);
	stmt->target = target;
	stmt->value = value;
	return stmt;
}

Stmt *p_stmt()
{
	Stmt *stmt = 0;
	(stmt = p_vardecl()) ||
	(stmt = p_assign()) ;
	return stmt;
}

Block *parse(Token *tokens)
{
	cur_token = tokens;
	cur_block = calloc(1, sizeof(Block));

	if(eat(TK_BOF)) {
		src_file_start = tokens[0].start;
	}
	else {
		error("INTERNAL: no BOF (beginning of file) present");
	}

	Stmt *first = 0;
	Stmt *last = 0;

	while(1) {
		Stmt *stmt = p_stmt();

		if(!stmt) {
			break;
		}

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

	if(!eat(TK_EOF)) {
		error("invalid statement");
	}

	return cur_block;
}