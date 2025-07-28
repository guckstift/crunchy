#include <stdio.h>
#include <stdarg.h>
#include "crunchy.h"

char runtime_src[] = {
	#include "../build/runtime.c.h"
	, 0
};

FILE *ofs = 0;
static int level = 0;

void gen_token(Token *token);
void gen_type(Type *type);
void gen_expr(Expr *expr);
void gen_stmt(Stmt *stmt);
void gen_local_block(Block *block);

void mod_gen_node(va_list args)
{
	void *node = va_arg(args, void*);
	Kind *kind = node;

	if(*kind > STMT_KIND_START)
		gen_stmt(node);
	else if(*kind > EXPR_KIND_START)
		gen_expr(node);
	else if(*kind > TYPE_KIND_START)
		gen_type(node);
	else
		gen_token(node);
}

void gen_token(Token *token)
{
	print("%S", token->start, token->length);
}

void gen_var(Token *ident, Block *parent_block)
{
	print("(frame%i.v_%n)", parent_block->id, ident);
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
			print("(%n != 0)", expr);
			break;
		default:
			print("/* INTERNAL: unknown expression to generate cast for */");
	}
}

void gen_expr(Expr *expr)
{
	if(expr->temp) {
		print("(frame%i.temp%i = ", expr->temp->parent_block->id, expr->temp->id);
	}

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
				if(expr->chars[i] == '"')
					print("\\\"");
				else
					print("%c", expr->chars[i]);
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
			if(expr->type->kind == TY_STRING)
				print("concat_strings(%n, %n)", expr->left, expr->right);
			else
				print("(%n%n%n)", expr->left, expr->op, expr->right);

			break;
		default:
			print("/* INTERNAL: unknown expression to generate */");
	}

	if(expr->temp) {
		print(")");
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

	if(value->type->kind == TY_BOOL)
		print("%n ? \"true\" : \"false\");\n", value);
	else
		print("%n);\n", value);
}

void gen_stmt(Stmt *stmt)
{
	print("%>");

	switch(stmt->kind) {
		case ST_VARDECL:
			gen_var(stmt->ident, stmt->parent_block);
			print(" = %n;\n", stmt->init);
			break;
		case ST_ASSIGN:
			print("%n = %n;\n", stmt->target, stmt->value);
			break;
		case ST_PRINT:
			gen_print(stmt->value);
			break;
		case ST_IF:
			print("if(%n) {\n", stmt->cond);
			gen_local_block(stmt->body);
			print("%>}\n");

			if(stmt->else_body) {
				print("%>else {\n");
				gen_local_block(stmt->else_body);
				print("%>}\n");
			}

			print("%>cur_frame = (Frame*)&frame%i;\n", stmt->parent_block->id);
			break;
		default:
			print("// INTERNAL: unknown statement to generate\n");
	}
}

void gen_decls(Block *block)
{
	print("%>struct {%+\n");
	print("%>void *parent;\n");
	print("%>int64_t num_gc_decls;\n");

	for(Temp *temp = block->temps; temp; temp = temp->next) {
		print("%>%n temp%i;\n", temp->type, temp->id);
	}

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl) {
		if(decl->type->kind == TY_STRING)
			print("%>%n v_%n;\n", decl->type, decl->ident);
	}

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl) {
		if(decl->type->kind != TY_STRING)
			print("%>%n v_%n;\n", decl->type, decl->ident);
	}

	Block *parent_block = block->parent;

	if(parent_block)
		print("%-%>} frame%i = {.parent = &frame%i", block->id, parent_block->id);
	else
		print("%-%>} frame%i = {.parent = 0", block->id);

	print(", .num_gc_decls = %iL};\n", block->num_gc_decls);
}

void gen_local_block(Block *block)
{
	print("%+");
	gen_decls(block);
	print("%>cur_frame = (Frame*)&frame%i;\n", block->id);
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) gen_stmt(stmt);
	print("%-");
}

void gen_block(Block *block)
{
	print("%+");
	print("%>cur_frame = (Frame*)&frame%i;\n", block->id);
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) gen_stmt(stmt);
	print("%-");
}

void generate(Block *block, char *output_file)
{
	Stmt *stmts = block->stmts;
	ofs = fopen(output_file, "wb");
	set_print_file(ofs);
	set_escape_mod('n', mod_gen_node);

	print("%s\n\n", runtime_src);
	gen_decls(block);
	print("int main(int argc, char **argv) {\n");
	gen_block(block);
	print("\treturn 0;\n");
	print("}\n");

	set_print_file(stdout);
	set_escape_mod('n', 0);
	fclose(ofs);
}