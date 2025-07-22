#include <stdio.h>
#include "crunchy.h"

static int level = 0;

void print_token(Token *token)
{
	fwrite(token->start, 1, token->length, stdout);
}

void print_tokens(Token *tokens)
{
	for(Token *token = tokens; token->kind != TK_EOF; token ++) {
		printf("%s ",
			token->kind == TK_BOF     ? "<BOF>     " :
			token->kind == TK_EOF     ? "<EOF>     " :
			token->kind == TK_INT     ? "<INT>     " :
			token->kind == TK_IDENT   ? "<IDENT>   " :
			token->kind == TK_STRING  ? "<STRING>  " :

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

		print_token(token);
		printf("\n");
	}
}

void print_indent()
{
	for(int i=0; i<level; i++) {
		printf("  ");
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
		case TY_STRING:
			printf("string");
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
		case EX_STRING:
			printf("\"");

			for(int64_t i=0; i < expr->length; i++) {
				if(expr->chars[i] == '"') {
					printf("\\\"");
				}
				else {
					fputc(expr->chars[i], stdout);
				}
			}

			printf("\"");
			break;
		case EX_VAR:
			print_token(expr->ident);
			break;
		case EX_CAST:
			print_type(expr->type);
			printf("(");
			print_expr(expr->subexpr);
			printf(")");
			break;
		case EX_BINOP:
			printf("(");
			print_expr(expr->left);
			print_token(expr->op);
			print_expr(expr->right);
			printf(")");
			break;
		default:
			printf("<unknown-expr:%i>", expr->kind);
			break;
	}
}

void print_stmt(Stmt *stmt)
{
	print_indent();

	switch(stmt->kind) {
		case ST_VARDECL:
			printf("var ");
			print_token(stmt->ident);

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
		case ST_PRINT:
			printf("print ");
			print_expr(stmt->value);
			printf(";\n");
			break;
		case ST_ASSIGN:
			print_expr(stmt->target);
			printf(" = ");
			print_expr(stmt->value);
			printf(";\n");
			break;
		case ST_IF:
			printf("if ");
			print_expr(stmt->cond);
			printf(" {\n");
			level ++;
			print_block(stmt->body);
			level --;
			print_indent();
			printf("}\n");
			break;
		default:
			printf("<unknown-stmt>\n");
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
	print_indent();
	printf("# scope: ");

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl) {
		print_token(decl->ident);
		printf("; ");
	}

	printf("\n");
	print_stmts(block->stmts);
}