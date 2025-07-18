#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "crunchy.h"

void a_block(Block *block);

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

	if(expr->type->kind == TY_STRING) {
		error_at(expr->start, "strings can not be converted to some other type");
	}

	if(expr->kind == EX_VAR) {
		Expr *cast = new_expr(EX_CAST, expr->start, 0);
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
		case TY_STRING:
			error_at(expr->start, "can not convert other types to string");
			break;
		default:
			error_at(expr->start, "INTERNAL: unknown type to adjust expression to");
	}

	return expr;
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

Stmt *lookup(Token *ident)
{
	return lookup_in(ident, cur_block);
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
		case EX_STRING:
			expr->type = new_type(TY_STRING);
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
		case EX_BINOP:
			a_expr(expr->left);
			a_expr(expr->right);
			expr->left = adjust_expr_to_type(expr->left, new_type(TY_INT));
			expr->right = adjust_expr_to_type(expr->right, new_type(TY_INT));
			expr->type = new_type(TY_INT);
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
		case ST_PRINT:
			a_expr(stmt->value);
			break;
		case ST_ASSIGN:
			a_expr(stmt->target);

			if(!stmt->target->is_lvalue) {
				error_at(stmt->target->start, "target is not assignable");
			}

			a_expr(stmt->value);
			stmt->value = adjust_expr_to_type(stmt->value, stmt->target->type);
			break;
		case ST_IF:
			a_expr(stmt->cond);
			stmt->cond = adjust_expr_to_type(stmt->cond, new_type(TY_BOOL));
			a_block(stmt->body);
			break;
		default:
			error_at(stmt->start, "INTERNAL: unknown statement to analyse");
	}
}

void a_block(Block *block)
{
	Block *old_block = cur_block;
	cur_block = block;

	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		a_stmt(stmt);
	}

	cur_block = old_block;
}

void analyse(Block *block)
{
	a_block(block);
}