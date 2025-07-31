#include <stdlib.h>
#include <string.h>
#include "crunchy.h"

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

	error_at(expr->start, "can not convert %n to %n", expr->type, type);
}