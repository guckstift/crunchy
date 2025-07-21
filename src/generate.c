#include <stdio.h>
#include "crunchy.h"

char runtime_src[] = {
	#include "../build/runtime.c.h"
	, 0
};

FILE *ofs = 0;
static int level = 0;

void gen_expr(Expr *expr);
void gen_local_block(Block *block);

void gen_indent()
{
	for(int i=0; i<level; i++) {
		fprintf(ofs, "\t");
	}
}

void gen_token(Token *token)
{
	fwrite(token->start, 1, token->length, ofs);
}

void gen_type(Type *type)
{
	switch(type->kind) {
		case TY_INT:
			fprintf(ofs, "int64_t");
			break;
		case TY_BOOL:
			fprintf(ofs, "uint8_t");
			break;
		case TY_STRING:
			fprintf(ofs, "String");
			break;
		default:
			fprintf(ofs, "/* INTERNAL: unknown type to generate */");
	}
}

void gen_cast(Type *type, Expr *expr)
{
	switch(type->kind) {
		case TY_INT:
			gen_expr(expr);
			break;
		case TY_BOOL:
			fprintf(ofs, "(");
			gen_expr(expr);
			fprintf(ofs, " != 0)");
			break;
		default:
			fprintf(ofs, "/* INTERNAL: unknown expression to generate cast for */");
	}
}

void gen_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			fprintf(ofs, "%liL", expr->ival);
			break;
		case EX_BOOL:
			fprintf(ofs, "%li", expr->ival);
			break;
		case EX_STRING:
			fprintf(ofs, "(String){.length = %liL, .chars = \"", expr->length);

			for(int64_t i=0; i < expr->length; i++) {
				if(expr->chars[i] == '"') {
					fprintf(ofs, "\\\"");
				}
				else {
					fputc(expr->chars[i], ofs);
				}
			}

			fprintf(ofs, "\"}");
			break;
		case EX_VAR:
			gen_token(expr->ident);
			break;
		case EX_CAST:
			gen_cast(expr->type, expr->subexpr);
			break;
		case EX_BINOP:
			fprintf(ofs, "(");
			gen_expr(expr->left);
			gen_token(expr->op);
			gen_expr(expr->right);
			fprintf(ofs, ")");
			break;
		default:
			fprintf(ofs, "/* INTERNAL: unknown expression to generate */");
	}
}

void gen_print(Expr *value)
{
	switch(value->type->kind) {
		case TY_INT:
			fprintf(ofs, "printf(\"%%li\\n\", ");
			break;
		case TY_BOOL:
			fprintf(ofs, "printf(\"%%s\\n\", ");
			break;
		case TY_STRING:
			fprintf(ofs, "print_string(");
			break;
		default:
			fprintf(ofs, "// INTERNAL: unknown value type to generate print for\n");
			return;
	}

	gen_expr(value);

	if(value->type->kind == TY_BOOL) {
		fprintf(ofs, " ? \"true\" : \"false\");\n");
	}
	else {
		fprintf(ofs, ");\n");
	}
}

void gen_stmt(Stmt *stmt)
{
	gen_indent();

	switch(stmt->kind) {
		case ST_VARDECL:
			gen_token(stmt->ident);
			fprintf(ofs, " = ");
			gen_expr(stmt->init);
			fprintf(ofs, ";\n");
			break;
		case ST_ASSIGN:
			gen_expr(stmt->target);
			fprintf(ofs, " = ");
			gen_expr(stmt->value);
			fprintf(ofs, ";\n");
			break;
		case ST_PRINT:
			gen_print(stmt->value);
			break;
		case ST_IF:
			fprintf(ofs, "if(");
			gen_expr(stmt->cond);
			fprintf(ofs, ") {\n");
			gen_local_block(stmt->body);
			gen_indent();
			fprintf(ofs, "}\n");
			break;
		default:
			fprintf(ofs, "// INTERNAL: unknown statement to generate\n");
	}
}

void gen_decls(Block *block)
{
	for(Stmt *decl = block->decls; decl; decl = decl->next_decl) {
		gen_indent();
		gen_type(decl->type);
		fprintf(ofs, " ");
		gen_token(decl->ident);
		fprintf(ofs, ";\n");
	}
}

void gen_local_block(Block *block)
{
	level ++;
	gen_decls(block);

	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		gen_stmt(stmt);
	}

	level --;
}

void gen_block(Block *block)
{
	level ++;

	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		gen_stmt(stmt);
	}

	level --;
}

void generate(Block *block, char *output_file)
{
	Stmt *stmts = block->stmts;
	ofs = fopen(output_file, "wb");
	fprintf(ofs, "%s\n\n", runtime_src);
	gen_decls(block);
	fprintf(ofs, "int main(int argc, char **argv) {\n");
	gen_block(block);
	fprintf(ofs, "\treturn 0;\n");
	fprintf(ofs, "}\n");
	fclose(ofs);
}