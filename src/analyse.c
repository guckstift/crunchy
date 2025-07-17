#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "crunchy.h"

static Block *cur_block = 0;

void error_at(Token *at, char *msg)
{
	printf("error: %s\n", msg);
	Token *bof = at;

	if(!at) {
		exit(EXIT_FAILURE);
	}

	while(bof->kind != TK_BOF) {
		bof --;
	}

	char *src_file_start = bof->start;
	printf("%li: ", at->line);
	char *p = at->start;

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

Expr *get_default_value(Type *type)
{
	Expr *expr = 0;

	switch(type->kind) {
		case TY_INT:
			expr = new_expr(EX_INT, 0);
			expr->type = type;
			expr->ival = 0;
			break;
		case TY_BOOL:
			expr = new_expr(EX_BOOL, 0);
			expr->type = type;
			expr->ival = 0;
			break;
		default:
			error("INTERNAL: unknown type to get default value for");
	}

	return expr;
}

Expr *adjust_expr_to_type(Expr *expr, Type *type)
{
	if(expr->type->kind == type->kind) {
		return expr;
	}

	if(expr->kind == EX_VAR) {
		Expr *cast = new_expr(EX_CAST, expr->start);
		cast->type = type;
		cast->subexpr = expr;
		return cast;
	}

	switch(type->kind) {
		case TY_INT:
			expr->kind = type->kind;
			break;
		case TY_BOOL:
			expr->kind = type->kind;
			expr->ival = expr->ival != 0;
			break;
		default:
			error_at(expr->start, "INTERNAL: unknown type to adjust expression to");
	}

	return expr;
}

Stmt *lookup(Token *ident)
{
	for(Stmt *d = cur_block->decls; d; d = d->next_decl) {
		if(d->ident->length == ident->length && memcmp(d->ident->start, ident->start, d->ident->length) == 0) {
			return d;
		}
	}

	return 0;
}

void a_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			expr->type = new_type(TY_INT);
			break;
		case EX_BOOL:
			expr->type = new_type(TY_BOOL);
			break;
		case EX_VAR:
			expr->decl = lookup(expr->ident);

			if(!expr->decl) {
				error_at(expr->start, "could not find variable");
			}

			if(expr->start < expr->decl->end) {
				error_at(expr->start, "variable used before its declaration");
			}

			expr->type = expr->decl->type;
			break;
		default:
			error_at(expr->start, "INTERNAL: unknown expression to analyse");
	}
}

void a_stmt(Stmt *stmt)
{
	switch(stmt->kind) {
		case ST_VARDECL:
			if(stmt->init) {
				a_expr(stmt->init);

				if(!stmt->type) {
					stmt->type = stmt->init->type;
				}
				else {
					stmt->init = adjust_expr_to_type(stmt->init, stmt->type);
				}
			}
			else if(stmt->type) {
				stmt->init = get_default_value(stmt->type);
			}

			if(!stmt->type) {
				error_at(stmt->start, "could not find out the type for this variable declaration");
			}

			break;
		default:
			error_at(stmt->start, "INTERNAL: unknown statement to analyse");
	}
}

void analyse(Block *block)
{
	cur_block = block;

	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		a_stmt(stmt);
	}
}