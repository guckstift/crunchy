#include <stdio.h>
#include "crunchy.h"

FILE *ofs = 0;

void gen_expr(Expr *expr);

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
		case EX_VAR:
			gen_token(expr->ident);
			break;
		case EX_CAST:
			gen_cast(expr->type, expr->subexpr);
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
	fprintf(ofs, "\t");

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
		default:
			fprintf(ofs, "// INTERNAL: unknown statement to generate\n");
	}
}

void generate(Block *block, char *output_file)
{
	Stmt *stmts = block->stmts;
	ofs = fopen(output_file, "wb");
	fprintf(ofs, "#include <stdint.h>\n");
	fprintf(ofs, "#include <stdio.h>\n");

	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		if(stmt->kind == ST_VARDECL) {
			gen_type(stmt->type);
			fprintf(ofs, " ");
			gen_token(stmt->ident);
			fprintf(ofs, ";\n");
		}
	}

	fprintf(ofs, "int main(int argc, char **argv) {\n");

	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		gen_stmt(stmt);
	}

	fprintf(ofs, "\treturn 0;\n");
	fprintf(ofs, "}\n");
	fclose(ofs);
}