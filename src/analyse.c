#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "crunchy.h"

void a_block(Block *block);
void a_expr(Expr *expr);

static Block *cur_block = 0;

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
			if(!expr->decl) error_at(expr->start, "could not find %n", expr->ident);
			if(expr->start < expr->decl->end) error_at(expr->start, "%n is used before its declaration", expr->ident);
			expr->type = expr->decl->type;
			break;
		case EX_BINOP:
			a_binop(expr);
			if(expr->type->kind == TY_STRING) expr->temp = declare_temp(expr->type);
			break;
		case EX_CALL:
			a_expr(expr->callee);
			expr->type = new_type(TY_VOID);
			break;

		case EX_ARRAY: {
			Type *itemtype = 0;

			for(Expr *item = expr->items; item; item = item->next) {
				a_expr(item);
				if(itemtype) item = adjust_expr_to_type(item, itemtype);
				else itemtype = item->type;
			}

			if(!itemtype) itemtype = new_type(TY_UNKNOWN);
			expr->type = new_type(TY_ARRAY);
			expr->type->subtype = itemtype;
		} break;

		default:
			error_at(expr->start, "INTERNAL: unknown expression to analyse");
	}
}

void validate_vardecl_type(Stmt *vardecl, Type *type)
{
	if(type->kind == TY_VOID)
		error_at(vardecl->start, "type is empty");

	if(type->kind == TY_UNKNOWN)
		error_at(vardecl->start, "type is incomplete");

	if(type->kind == TY_ARRAY)
		validate_vardecl_type(vardecl, type->subtype);
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

			if(!stmt->type)
				error_at(stmt->start, "could not find out the type for this variable declaration");

			if(stmt->type->kind == TY_STRING || stmt->type->kind == TY_ARRAY)
				cur_block->num_gc_decls ++;

			validate_vardecl_type(stmt, stmt->type);
			break;
		case ST_FUNCDECL:
			a_block(stmt->body);
			stmt->type = new_type(TY_FUNC);
			break;
		case ST_PRINT:
			a_expr(stmt->value);
			break;
		case ST_ASSIGN:
			a_expr(stmt->target);

			if(!stmt->target->is_lvalue) {
				error_at(stmt->target->start, "this target is not assignable");
			}

			a_expr(stmt->value);
			stmt->value = adjust_expr_to_type(stmt->value, stmt->target->type);
			break;
		case ST_CALL:
			a_expr(stmt->call);
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