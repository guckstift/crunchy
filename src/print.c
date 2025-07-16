#include <stdio.h>
#include "crunchy.h"

void print_tokens(Token *tokens)
{
	for(Token *token = tokens; token->kind != TK_EOF; token ++) {
		printf("%s ",
			token->kind == TK_INT   ? "<INT>     " :
			token->kind == TK_IDENT ? "<IDENT>   " :

			#define _(a) \
			token->kind == KW_ ## a ? "<KEYWORD> " :
			KEYWORDS
			#undef _

			#define _(a, b) \
			token->kind == PT_ ## b ? "<PUNCT>   " :
			PUNCTS
			#undef _

			"<unknown-token>"
		);

		fwrite(token->start, 1, token->length, stdout);
		printf("\n");
	}
}

void print_type(Type *type)
{
	switch(type->kind) {
		case TY_INT:
			printf("int");
			break;
		case TY_BOOL:
			printf("bool");
			break;
		default:
			printf("<unknown-type>");
			break;
	}
}

void print_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			printf("%li", expr->ival);
			break;
		case EX_BOOL:
			printf("%s", expr->ival ? "true" : "false");
			break;
		case EX_VAR:
			fwrite(expr->ident->start, 1, expr->ident->length, stdout);
			break;
		default:
			printf("<unknown-expr>");
			break;
	}
}

void print_stmt(Stmt *stmt)
{
	switch(stmt->kind) {
		case ST_VARDECL:
			printf("var ");
			fwrite(stmt->ident->start, 1, stmt->ident->length, stdout);

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
		default:
			printf("<unknown-stmt>");
			break;
	}
}

void print_stmts(Stmt *stmts)
{
	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		print_stmt(stmt);
	}
}

void print_block(Block *block)
{
	printf("# scope: ");

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl) {
		fwrite(decl->ident->start, 1, decl->ident->length, stdout);
		printf("; ");
	}

	printf("\n");
	print_stmts(block->stmts);
}