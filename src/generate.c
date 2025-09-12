#include <stdio.h>
#include <stdarg.h>
#include "crunchy.h"

/*
char runtime_src[] = {
	#include "../build/runtime.c.h"
	, 0
};
*/

FILE *ofs = 0;
static int level = 0;

void gen_expr(Expr *expr);
void gen_block(Block *block);
void gen_print(Expr *value);
void gen_type_desc_name(Type *type);

void gen_token(Token *token)
{
	print("%S", token->start, token->length);
}

void gen_full_name(Stmt *decl)
{
	if(decl->kind == ST_FUNCDECL)
		print("v_%n", decl->ident);
	else
		print("(frame%i.v_%n)", decl->parent_block->id, decl->ident);
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
		case TY_FUNC:
			print("Function");
			break;
		case TY_ARRAY:
			print("Array*");
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
		case EX_NOOPFUNC:
			print("noop");
			break;
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
			gen_full_name(expr->decl);
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
		case EX_CALL:
			print("%n()", expr->callee);
			break;
		case EX_ARRAY:
			print("new_array(&");
			gen_type_desc_name(expr->type);
			print(", %i, &(%n[]){", expr->length, expr->type->subtype);

			for(Expr *item = expr->items; item; item = item->next) {
				print("%n, ", item);
			}

			print("})");
			break;
		default:
			print("/* INTERNAL: unknown expression to generate */");
	}

	if(expr->temp) {
		print(")");
	}
}

void gen_print_array(Expr *value)
{
	if(value->kind == EX_ARRAY) {
		print("%>printf(\"[\");\n");

		for(Expr *item = value->items; item; item = item->next) {
			if(item != value->items) print("%>printf(\", \");\n");
			gen_print(item);
		}

		print("%>printf(\"]\");\n");
	}
	else {
		print("%>print_array(%n, &", value);
		gen_type_desc_name(value->type);
		print(");\n");
	}
}

void gen_print(Expr *value)
{
	switch(value->type->kind) {
		case TY_INT:
			print("%>printf(\"%%li\", ");
			break;
		case TY_BOOL:
			print("%>printf(\"%%s\", ");
			break;
		case TY_STRING:
			print("%>print_string(");
			break;
		case TY_ARRAY:
			gen_print_array(value);
			return;
		default:
			print("%>// INTERNAL: unknown value type to generate print for\n");
			return;
	}

	if(value->type->kind == TY_BOOL)
		print("%n ? \"true\" : \"false\");\n", value);
	else
		print("%n);\n", value);
}

void gen_print_line(Expr *value)
{
	gen_print(value);
	print("%>printf(\"\\n\");\n");
}

void gen_stmt(Stmt *stmt)
{
	switch(stmt->kind) {
		case ST_VARDECL:
			print("%>");
			gen_full_name(stmt);
			print(" = %n;\n", stmt->init);
			break;
		case ST_FUNCDECL:
			break;
		case ST_ASSIGN:
			print("%>%n = %n;\n", stmt->target, stmt->value);
			break;
		case ST_CALL:
			print("%>%n;\n", stmt->call);
			break;
		case ST_PRINT:
			gen_print_line(stmt->value);
			break;
		case ST_IF:
			print("%>if(%n) {%+\n", stmt->cond);
			gen_block(stmt->body);
			print("%-%>}\n");

			if(stmt->else_body) {
				print("%>else {%+\n");
				gen_block(stmt->else_body);
				print("%-%>}\n");
			}

			break;
		default:
			print("%>// INTERNAL: unknown statement to generate\n");
	}
}

void gen_type_desc_data(Type *type)
{
	print("{.kind = TY_");

	switch(type->kind) {
		case TY_INT: print("INT"); break;
		case TY_BOOL: print("BOOL"); break;
		case TY_STRING: print("STRING"); break;
		case TY_FUNC: print("FUNC"); break;
		case TY_ARRAY: print("ARRAY"); break;
		default: print("/* invalid type to generate type desc for */");
	}

	if(type->kind == TY_ARRAY) {
		print(", .subtype = &");
		gen_type_desc_name(type->subtype);
	}

	print("}");
}

void gen_type_desc_name(Type *type)
{
	print("t_");

	for(Type *type_node = type; type_node; type_node = type_node->subtype) {
		if(type_node->kind == TY_ARRAY) print("array_");
		else print_type(type_node);
	}
}

void gen_type_desc(Type *type)
{
	print("%>Type ");
	gen_type_desc_name(type);
	print(" = ");
	gen_type_desc_data(type);
	print(";\n");
}

void gen_decls(Block *block)
{
	for(Type *type = block->types; type; type = type->next) {
		gen_type_desc(type);
	}

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl) {
		if(decl->kind == ST_FUNCDECL)
			print("%>void v_%n();\n", decl->ident);
	}

	print("%>struct {%+\n");
	print("%>void *parent;\n");
	print("%>int64_t num_gc_decls;\n");

	for(Temp *temp = block->temps; temp; temp = temp->next) {
		print("%>%n temp%i;\n", temp->type, temp->id);
	}

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl) {
		if(decl->kind != ST_FUNCDECL && is_gc_type(decl->type))
			print("%>%n v_%n;\n", decl->type, decl->ident);
	}

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl) {
		if(decl->kind != ST_FUNCDECL && !is_gc_type(decl->type))
			print("%>%n v_%n;\n", decl->type, decl->ident);
	}

	print(
		"%-%>} frame%i = {.parent = %s, .num_gc_decls = %iL};\n",
		block->id, block->parent ? "get_cur_frame()" : "0", block->num_gc_decls
	);

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl) {
		if(decl->kind == ST_FUNCDECL) {
			print("void v_%n() {%+\n", decl->ident);
			gen_block(decl->body);
			print("%-}\n");
		}
	}
}

void gen_block(Block *block)
{
	if(block->parent) gen_decls(block);
	print("%>push_frame(&frame%i);\n", block->id);
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) gen_stmt(stmt);
	print("%>pop_frame();\n");
}

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

void generate(Block *block, char *output_file)
{
	Stmt *stmts = block->stmts;
	ofs = fopen(output_file, "wb");
	set_print_file(ofs);
	set_escape_mod('n', mod_gen_node);

	//print("%s\n\n", runtime_src);
	//print("#include \"src/runtime.c\"\n");
	print("#include \"runtime.h\"\n");
	gen_decls(block);
	print("int main(int argc, char **argv) {%+\n");
	gen_block(block);
	print("%>return 0;\n");
	print("%-}\n");

	set_print_file(stdout);
	set_escape_mod('n', 0);
	fclose(ofs);
}