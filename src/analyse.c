#include <stdlib.h>
#include "crunchy.h"

Expr *get_default_value(Type *type)
{
	Expr *expr = 0;

	switch(type->kind) {
		case TY_INT:
			expr = malloc(sizeof(Expr));
			expr->kind = EX_INT;
			expr->type = type;
			expr->ival = 0;
			break;
		case TY_BOOL:
			expr = malloc(sizeof(Expr));
			expr->kind = EX_BOOL;
			expr->type = type;
			expr->ival = 0;
			break;
		default:
			error("INTERNAL: unknown type to get default value for");
	}
}

Expr *adjust_expr_to_type(Expr *expr, Type *type)
{
	if(expr->type->kind == type->kind) {
		return expr;
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
			error("INTERNAL: unknown type to adjust expression to");
	}

	return expr;
}

void a_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			expr->type = malloc(sizeof(Type));
			expr->type->kind = TY_INT;
			break;
		case EX_BOOL:
			expr->type = malloc(sizeof(Type));
			expr->type->kind = TY_BOOL;
			break;
		default:
			error("INTERNAL: unknown expression to analyse");
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
				error("could not find out the type for this variable declaration");
			}

			break;
		default:
			error("INTERNAL: unknown statement to analyse");
	}
}

void analyse(Stmt *stmts)
{
	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		a_stmt(stmt);
	}
}