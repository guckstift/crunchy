#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "crunchy.h"

#define error(...) parse_error(__VA_ARGS__)

Block *p_block();

static Token *cur_token = 0;
static Block *cur_block = 0;
static int64_t next_block_id = 0;

void parse_error(char *msg)
{
	Token *bof = cur_token;
	while(bof->kind != TK_BOF) bof --;
	char *src_file_start = bof->start;
	print("%[f00]error:%[] %s\n", msg);
	print("%[888]");
	int64_t offset = print("%i", cur_token->line);
	offset += print(":%[] ");
	char *line_start = cur_token->start;
	while(line_start > src_file_start && line_start[-1] != '\n') line_start --;

	for(char *p = line_start; *p && *p != '\n'; p++) {
		if(*p == '\t') {
			if(p < cur_token->start)
				offset += fprintf(stdout, "  ");
			else
				fprintf(stdout, "  ");
		}
		else {
			fputc(*p, stdout);
			if(p < cur_token->start) offset ++;
		}
	}

	printf("\n");
	for(int64_t i=0; i < offset; i++) printf(" ");
	print("%[f00]^%[]\n");
	exit(EXIT_FAILURE);
}

Type *new_type(Kind kind)
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

Expr *new_expr(Kind kind, Token *start, uint8_t is_lvalue)
{
	Expr *expr = calloc(1, sizeof(Expr));
	expr->kind = kind;
	expr->start = start;
	expr->is_lvalue = is_lvalue;
	return expr;
}

Stmt *new_stmt(Kind kind, Block *parent, Token *start, Token *end)
{
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->kind = kind;
	stmt->next = 0;
	stmt->parent_block = parent;
	stmt->start = start;
	stmt->end = end;
	return stmt;
}

int declare_in(Stmt *decl, Block *block)
{
	for(Stmt *d = block->decls; d; d = d->next_decl) {
		if(
			d->ident->length == decl->ident->length &&
			memcmp(d->ident->start, decl->ident->start, d->ident->length) == 0
		) {
			return 0;
		}
	}

	if(block->decls) {
		block->last_decl->next_decl = decl;
		block->last_decl = decl;
	}
	else {
		block->decls = decl;
		block->last_decl = decl;
	}

	return 1;
}

int declare(Stmt *decl)
{
	return declare_in(decl, cur_block);
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

Type *p_type()
{
	if(eat(KW_int)) return new_type(TY_INT);
	else if(eat(KW_bool)) return new_type(TY_BOOL);
	else if(eat(KW_string)) return new_type(TY_STRING);
	return 0;
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
	if(!left) return 0;

	while(1) {
		Token *op = eat(PT_PLUS);
		if(!op) return left;
		Expr *right = p_atom();
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
	stmt->next_decl = 0;
	if(!declare(stmt)) error("variable is already declared");
	expect(PT_SEMICOLON, "missing semicolon after variable declaration");
	stmt->parent_block = cur_block;
	stmt->end = cur_token;
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

Stmt *p_assign()
{
	Expr *target = p_expr();
	if(!target) return 0;
	expect(PT_EQUALS, "expected '=' after assignment target");
	Expr *value = p_expr();
	if(!value) error("expected assignment value after '='");
	expect(PT_SEMICOLON, "missing semicolon after variable declaration");
	Stmt *stmt = new_stmt(ST_ASSIGN, cur_block, target->start, cur_token);
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