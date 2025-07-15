#include <stdlib.h>
#include "crunchy.h"

void a_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			expr->type = malloc(sizeof(Type));
			expr->type->kind = TY_INT;
			break;
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
			}
			else if(stmt->type) {
				stmt->init = malloc(sizeof(Expr));
				stmt->init->kind = EX_INT;
				stmt->init->type = stmt->type;
				stmt->init->ival = 0;
			}

			break;
	}
}

void analyse(Stmt *stmts)
{
	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		a_stmt(stmt);
	}
}