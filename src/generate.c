#include <stdio.h>
#include "crunchy.h"

FILE *ofs = 0;

void gen_type(Type *type)
{
	switch(type->kind) {
		case TY_INT:
			fprintf(ofs, "int64_t");
			break;
	}
}

void gen_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			fprintf(ofs, "%li", expr->ival);
			break;
	}
}

void gen_stmt(Stmt *stmt)
{
	fprintf(ofs, "\t");

	switch(stmt->kind) {
		case ST_VARDECL:
			fwrite(stmt->ident->start, 1, stmt->ident->end - stmt->ident->start, ofs);
			fprintf(ofs, " = ");
			gen_expr(stmt->init);
			fprintf(ofs, ";\n");
			break;
	}
}

void generate(Stmt *stmts, char *output_file)
{
	ofs = fopen(output_file, "wb");
	fprintf(ofs, "#include <stdint.h>\n");

	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		if(stmt->kind == ST_VARDECL) {
			gen_type(stmt->type);
			fprintf(ofs, " ");
			fwrite(stmt->ident->start, 1, stmt->ident->end - stmt->ident->start, ofs);
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