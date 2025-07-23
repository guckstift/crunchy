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
		print("\t");
	}
}

void gen_var(Token *ident, Block *parent_block)
{
	print("(frame%i.%n)", parent_block->id, ident);
}

void gen_type(Type *type)
{
	switch(type->kind) {
		case TY_INT:
			print("int64_t");
			break;
		case TY_BOOL:
			print("uint8_t");
			break;
		case TY_STRING:
			print("String*");
			break;
		default:
			print("/* INTERNAL: unknown type to generate */");
	}
}

void gen_cast(Type *type, Expr *expr)
{
	switch(type->kind) {
		case TY_INT:
			gen_expr(expr);
			break;
		case TY_BOOL:
			print("(");
			gen_expr(expr);
			print(" != 0)");
			break;
		default:
			print("/* INTERNAL: unknown expression to generate cast for */");
	}
}

void gen_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			print("%iL", expr->ival);
			break;
		case EX_BOOL:
			print("%i", expr->ival);
			break;
		case EX_STRING:
			print("new_string(%iL, \"", expr->length);

			for(int64_t i=0; i < expr->length; i++) {
				if(expr->chars[i] == '"') {
					print("\\\"");
				}
				else {
					print("%c", expr->chars[i]);
				}
			}

			print("\")");
			break;
		case EX_VAR:
			gen_var(expr->ident, expr->decl->parent_block);
			break;
		case EX_CAST:
			gen_cast(expr->type, expr->subexpr);
			break;
		case EX_BINOP:
			print("(");
			gen_expr(expr->left);
			print("%n", expr->op);
			gen_expr(expr->right);
			print(")");
			break;
		default:
			print("/* INTERNAL: unknown expression to generate */");
	}
}

void gen_print(Expr *value)
{
	switch(value->type->kind) {
		case TY_INT:
			print("printf(\"%%li\\n\", ");
			break;
		case TY_BOOL:
			print("printf(\"%%s\\n\", ");
			break;
		case TY_STRING:
			print("print_string(");
			break;
		default:
			print("// INTERNAL: unknown value type to generate print for\n");
			return;
	}

	gen_expr(value);

	if(value->type->kind == TY_BOOL) {
		print(" ? \"true\" : \"false\");\n");
	}
	else {
		print(");\n");
	}
}

void gen_stmt(Stmt *stmt)
{
	gen_indent();

	switch(stmt->kind) {
		case ST_VARDECL:
			gen_var(stmt->ident, stmt->parent_block);
			print(" = ");
			gen_expr(stmt->init);
			print(";\n");
			break;
		case ST_ASSIGN:
			gen_expr(stmt->target);
			print(" = ");
			gen_expr(stmt->value);
			print(";\n");
			break;
		case ST_PRINT:
			gen_print(stmt->value);
			break;
		case ST_IF:
			print("if(");
			gen_expr(stmt->cond);
			print(") {\n");
			gen_local_block(stmt->body);
			gen_indent();
			print("}\n");
			gen_indent();
			print("cur_frame = (Frame*)&frame%i;\n", stmt->parent_block->id);
			break;
		default:
			print("// INTERNAL: unknown statement to generate\n");
	}
}

void gen_decls(Block *block)
{
	gen_indent();
	print("struct {\n");
	level ++;
	gen_indent();
	print("void *parent;\n");
	gen_indent();
	print("int64_t num_gc_decls;\n");

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl) {
		if(decl->type->kind == TY_STRING) {
			gen_indent();
			gen_type(decl->type);
			print(" %n;\n", decl->ident);
		}
	}

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl) {
		if(decl->type->kind != TY_STRING) {
			gen_indent();
			gen_type(decl->type);
			print(" %n;\n", decl->ident);
		}
	}

	level --;
	gen_indent();
	Block *parent_block = block->parent;

	if(parent_block)
		print("} frame%i = {.parent = &frame%i", block->id, parent_block->id);
	else
		print("} frame%i = {.parent = 0", block->id);

	print(", .num_gc_decls = %iL};\n", block->num_gc_decls);
}

void gen_local_block(Block *block)
{
	level ++;
	gen_decls(block);
	gen_indent();
	print("cur_frame = (Frame*)&frame%i;\n", block->id);

	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		gen_stmt(stmt);
	}

	level --;
}

void gen_block(Block *block)
{
	level ++;
	gen_indent();
	print("cur_frame = (Frame*)&frame%i;\n", block->id);

	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		gen_stmt(stmt);
	}

	level --;
}

void generate(Block *block, char *output_file)
{
	Stmt *stmts = block->stmts;
	ofs = fopen(output_file, "wb");
	set_print_file(ofs);
	print("%s\n\n", runtime_src);
	gen_decls(block);
	print("int main(int argc, char **argv) {\n");
	gen_block(block);
	print("\treturn 0;\n");
	print("}\n");
	set_print_file(stdout);
	fclose(ofs);
}