#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "crunchy.h"

void a_block(Block *block);
void a_expr(Expr *expr);

static Block *cur_block = 0;

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
	Type *expr_type = expr->type;

	if(expr_type->kind == type->kind)
		return expr;

	if(expr_type->kind == TY_STRING)
		error_at(expr->start, "strings can not be converted to some other type");

	if(type->kind == TY_STRING)
		error_at(expr->start, "can not convert other types to string");

	if(expr->kind == EX_VAR) {
		Expr *cast = new_expr(EX_CAST, expr->start, 0);
		cast->type = type;
		cast->subexpr = expr;
		return cast;
	}

	switch(type->kind) {
		case TY_INT:
			expr->kind = EX_INT;
			break;
		case TY_BOOL:
			expr->kind = EX_BOOL;
			expr->ival = expr->ival != 0;
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

Temp *declare_temp(Type *type)
{
	static int64_t next_temp_id = 1;
	Temp *temp = calloc(1, sizeof(Temp));
	temp->next = 0;
	temp->type = type;
	temp->parent_block = cur_block;
	temp->id = next_temp_id ++;
	if(cur_block->temps) cur_block->last_temp->next = temp;
	else cur_block->temps = temp;
	cur_block->last_temp = temp;
	cur_block->num_gc_decls ++;
	return temp;
}

void a_binop(Expr *binop)
{
	Expr *left = binop->left;
	Expr *right = binop->right;
	a_expr(left);
	a_expr(right);
	Type *ltype = left->type;
	Type *rtype = right->type;

	if(ltype->kind == TY_STRING && rtype->kind == TY_STRING) {
		binop->type = new_type(TY_STRING);
	}
	else {
		binop->left = adjust_expr_to_type(left, new_type(TY_INT));
		binop->right = adjust_expr_to_type(right, new_type(TY_INT));
		binop->type = new_type(TY_INT);
	}
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
			expr->temp = declare_temp(expr->type);
			break;
		case EX_VAR:
			expr->decl = lookup(expr->ident);
			if(!expr->decl) error_at(expr->start, "could not find variable");
			if(expr->start < expr->decl->end) error_at(expr->start, "variable used before its declaration");
			expr->type = expr->decl->type;
			break;
		case EX_BINOP:
			a_binop(expr);
			if(expr->type->kind == TY_STRING) expr->temp = declare_temp(expr->type);
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

			if(stmt->type->kind == TY_STRING) {
				cur_block->num_gc_decls ++;
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
			if(stmt->else_body) a_block(stmt->else_body);
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