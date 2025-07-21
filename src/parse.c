#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "crunchy.h"

#define error(...) parse_error(__VA_ARGS__)

Block *p_block();

static char *src_file_start = 0;
static Token *cur_token = 0;
static Block *cur_block = 0;
static int64_t next_block_id = 0;

void parse_error(char *msg)
{
	printf("error: %s\n", msg);

	if(!src_file_start)
		exit(EXIT_FAILURE);

	printf("%li: ", cur_token->line);
	char *p = cur_token->start;

	while(p > src_file_start && p[-1] != '\n')
		p --;

	while(*p && *p != '\n') {
		fputc(*p, stdout);
		p ++;
	}

	printf("\n");
	exit(EXIT_FAILURE);
}

Type *new_type(TypeKind kind)
{
	if(kind == TY_INT) {
		static Type int_type = {.kind = TY_INT};
		return &int_type;
	}
	else if(kind == TY_BOOL) {
		static Type bool_type = {.kind = TY_BOOL};
		return &bool_type;
	}
	else if(kind == TY_STRING) {
		static Type string_type = {.kind = TY_STRING};
		return &string_type;
	}

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
		if(d->ident->length == decl->ident->length && memcmp(d->ident->start, decl->ident->start, d->ident->length) == 0)
			return 0;
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
	if(cur_token->kind == kind)
		return cur_token ++;

	return 0;
}

Type *p_type()
{
	Token *keyword = 0;
	TypeKind kind = TY_INVALID;

	if(eat(KW_int))
		kind = TY_INT;
	else if(eat(KW_bool))
		kind = TY_BOOL;
	else if(eat(KW_string))
		kind = TY_STRING;
	else
		return 0;

	return new_type(kind);
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

	return expr;
}

Expr *p_binop()
{
	Expr *left = p_atom();

	if(!left)
		return 0;

	while(1) {
		Token *op = eat(PT_PLUS);

		if(!op)
			return left;

		Expr *right = p_atom();

		if(!right)
			error("expected right side expression after +");

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

	if(!eat(KW_var))
		return 0;

	Token *ident = eat(TK_IDENT);

	if(!ident)
		error("missing variable name after var keyword");

	Type *type = 0;

	if(eat(PT_COLON)) {
		type = p_type();

		if(!type)
			error("missing type specification after colon");
	}

	Expr *init = 0;

	if(eat(PT_EQUALS)) {
		init = p_expr();

		if(!init)
			error("missing expression after equals");
	}

	if(!type && !init)
		error("a variable declaration must have at least either a type specification or an initializer expression");

	Stmt *stmt = new_stmt(ST_VARDECL, start, cur_token);
	stmt->ident = ident;
	stmt->type = type;
	stmt->init = init;
	stmt->next_decl = 0;

	if(!declare(stmt))
		error("variable is already declared");

	if(!eat(PT_SEMICOLON))
		error("missing semicolon after variable declaration");

	stmt->parent_block = cur_block;
	stmt->end = cur_token;
	return stmt;
}

Stmt *p_print()
{
	Token *start = cur_token;

	if(!eat(KW_print))
		return 0;

	Expr *value = p_expr();

	if(!value)
		error("missing expression to print");

	Stmt *stmt = new_stmt(ST_PRINT, start, cur_token);
	stmt->value = value;

	if(!eat(PT_SEMICOLON))
		error("missing semicolon after print statement");

	stmt->end = cur_token;
	return stmt;
}

Stmt *p_if()
{
	Token *start = cur_token;

	if(!eat(KW_if))
		return 0;

	Expr *cond = p_expr();

	if(!cond)
		error("missing condition after if keyword");

	if(!eat(PT_LCURLY))
		error("expected '{' after if-condition");

	Block *body = p_block();

	if(!eat(PT_RCURLY))
		error("expected '}' after if-body");

	Stmt *stmt = new_stmt(ST_IF, start, cur_token);
	stmt->cond = cond;
	stmt->body = body;
	return stmt;
}

Stmt *p_assign()
{
	Expr *target = p_expr();

	if(!target)
		return 0;

	if(!eat(PT_EQUALS))
		error("expected '=' after assignment target");

	Expr *value = p_expr();

	if(!value)
		error("expected assignment value after '='");

	if(!eat(PT_SEMICOLON))
		error("missing semicolon after variable declaration");

	Stmt *stmt = new_stmt(ST_ASSIGN, target->start, cur_token);
	stmt->target = target;
	stmt->value = value;
	return stmt;
}

Stmt *p_stmt()
{
	Stmt *stmt = 0;
	(stmt = p_vardecl()) ||
	(stmt = p_print()) ||
	(stmt = p_if()) ||
	(stmt = p_assign()) ;
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

		if(!stmt)
			break;

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

	if(eat(TK_BOF))
		src_file_start = tokens[0].start;
	else
		error("INTERNAL: no BOF (beginning of file) present");

	Block *main_block = p_block();

	if(!eat(TK_EOF))
		error("invalid statement");

	return main_block;
}