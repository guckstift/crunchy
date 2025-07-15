#include <stdio.h>
#include "crunchy.h"

void print_type(Type *type)
{
	switch(type->kind) {
		case TY_INT:
			printf("int");
			break;
	}
}

void print_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			printf("%li", expr->ival);
			break;
	}
}

void print_stmt(Stmt *stmt)
{
	switch(stmt->kind) {
		case ST_VARDECL:
			printf("var ");
			fwrite(stmt->ident->start, 1, stmt->ident->end - stmt->ident->start, stdout);

			if(stmt->type) {
				printf(" : ");
				print_type(stmt->type);
			}

			if(stmt->init) {
				printf(" = ");
				print_expr(stmt->init);
			}

			printf(";\n");
			break;
	}
}

void print(Stmt *stmts)
{
	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		print_stmt(stmt);
	}
}