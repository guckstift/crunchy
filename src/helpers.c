#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "crunchy.h"

void error(char *msg)
{
	set_print_file(stderr);
	print("%[f00]error:%[] %s\n", msg);
	exit(EXIT_FAILURE);
}

char *find_src_start(Token *token)
{
	Token *bof = token;
	while(bof->kind != TK_BOF) bof --;
	return bof->start;
}

char *find_line_start(char *ptr, char *src_start)
{
	while(ptr > src_start && ptr[-1] != '\n') ptr --;
	return ptr;
}

int64_t print_src_line(char *start, char *cursor, int64_t line)
{
	print("%[888]");
	int64_t offset = print("%i:%[] ", line);

	for(char *p = start; *p && *p != '\n'; p++) {
		if(*p == '\t') {
			if(p < cursor) offset += print("  ");
			else print("  ");
		}
		else {
			print("%c", *p);
			if(p < cursor) offset ++;
		}
	}

	print("\n");
	return offset;
}

void error_at(Token *at, char *msg, ...)
{
	set_print_file(stderr);
	va_list args;
	va_start(args, msg);
	print("%[f00]error:%[] ");
	vprint(msg, args);
	print("\n");
	va_end(args);
	char *src_start = find_src_start(at);
	char *line_start = find_line_start(at->start, src_start);
	int64_t offset = print_src_line(line_start, at->start, at->line);
	for(int64_t i=0; i < offset; i++) print(" ");
	print("%[f00]^%[]\n");
	exit(EXIT_FAILURE);
}

char *load_text_file(char *file_name)
{
	FILE *fs = fopen(file_name, "rb");

	if(!fs) {
		error("could not open input file");
	}

	fseek(fs, 0, SEEK_END);
	long size = ftell(fs);
	rewind(fs);
	char *text = malloc(size + 1);
	text[size] = 0;
	fread(text, 1, size, fs);
	fclose(fs);
	return text;
}

Type *new_type(Kind kind)
{
	if(kind == TY_UNKNOWN) {
		static Type unknown_type = {.kind = TY_UNKNOWN};
		return &unknown_type;
	}
	else if(kind == TY_INT) {
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

Stmt *lookup_in(Token *ident, Block *block)
{
	for(Stmt *d = block->decls; d; d = d->next_decl) {
		if(d->ident->length == ident->length && memcmp(d->ident->start, ident->start, d->ident->length) == 0) {
			return d;
		}
	}

	if(block->parent) {
		return lookup_in(ident, block->parent);
	}

	return 0;
}

Expr *get_default_value(Type *type)
{
	Expr *expr = 0;

	switch(type->kind) {
		case TY_INT:
			expr = new_expr(EX_INT, 0, 0);
			expr->type = type;
			expr->ival = 0;
			break;
		case TY_BOOL:
			expr = new_expr(EX_BOOL, 0, 0);
			expr->type = type;
			expr->ival = 0;
			break;
		case TY_STRING:
			expr = new_expr(EX_STRING, 0, 0);
			expr->type = type;
			expr->chars = "";
			expr->length = 0;
			break;
		case TY_FUNC:
			expr = new_expr(EX_NOOPFUNC, 0, 0);
			expr->type = type;
			break;
		case TY_ARRAY:
			expr = new_expr(EX_ARRAY, 0, 0);
			expr->type = type;
			break;
		default:
			error("INTERNAL: unknown type to get default value for");
	}

	return expr;
}

int types_equal(Type *a, Type *b)
{
	if(a == b)
		return 1;
	if(a->kind == TY_ARRAY && b->kind == TY_ARRAY)
		return types_equal(a->subtype, b->subtype);
	return a->kind == b->kind;
}

int is_gc_type(Type *type)
{
	return type->kind == TY_STRING || type->kind == TY_ARRAY;
}

Expr *adjust_expr_to_type(Expr *expr, Type *type)
{
	if(types_equal(expr->type, type))
		return expr;

	if(
		expr->type->kind == TY_INT && type->kind == TY_BOOL ||
		expr->type->kind == TY_BOOL && type->kind == TY_INT
	) {
		if(expr->kind == EX_INT) {
			expr->kind = EX_BOOL;
			expr->type = type;
			expr->ival = expr->ival != 0;
			return expr;
		}
		else if(expr->kind == EX_BOOL) {
			expr->kind = EX_INT;
			expr->type = type;
			return expr;
		}

		Expr *cast = new_expr(EX_CAST, expr->start, 0);
		cast->type = type;
		cast->subexpr = expr;
		return cast;
	}

	if(expr->type->kind == TY_ARRAY) {
		Type *inner_expr_type = expr->type;
		Type *inner_target_type = type;

		while(inner_expr_type->kind == TY_ARRAY && inner_target_type->kind == TY_ARRAY) {
			inner_expr_type = inner_expr_type->subtype;
			inner_target_type = inner_target_type->subtype;
		}

		if(inner_expr_type->kind == TY_UNKNOWN) {
			void *next_backup = inner_expr_type->next;
			*inner_expr_type = *inner_target_type;
			inner_expr_type->next = next_backup;
			return expr;
		}
	}

	error_at(expr->start, "can not convert %n to %n", expr->type, type);
}