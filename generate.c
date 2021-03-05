#include "crunchy.h"

static void gen_block(Node *node);
static void gen_expr(Node *node);

static void gen_type(Node *node)
{
	if(node->kind == ND_PRIMTYPE) {
		printf("%s", node->name);
	}
}

static void gen_ident(Token *token)
{
	printf("%s", token->text);
}

static void gen_prim(Node *node)
{
	Token *token = node->token;
	
	if(token->kind == TK_IDENT) {
		printf("%s", token->text);
	}
	else if(token->kind == TK_INT) {
		printf("%s", token->text);
	}
}

static void gen_chain(Node *node)
{
	printf("(");
	
	for(Node *cur = node; cur; cur = cur->next) {
		if(cur->kind == ND_CHAIN && cur->tier == node->tier) {
			gen_expr(cur->expr);
			printf(" %s ", cur->token->text);
		}
		else {
			gen_expr(cur);
		}
	}
	
	printf(")");
}

static void gen_expr(Node *node)
{
	if(node->kind == ND_PRIM) {
		gen_prim(node);
	}
	else if(node->kind == ND_CHAIN) {
		gen_chain(node);
	}
}

static void gen_vardecl(Node *node)
{
	gen_type(node->type);
	printf(" ");
	gen_ident(node->token);
	
	if(node->expr->isconst) {
		printf(" = ");
		gen_expr(node->expr);
	}
	
	printf(";\n");
}

static void gen_funcdecl(Node *node)
{
	printf("void ");
	gen_ident(node->token);
	printf("(){\n");
	gen_block(node->body);
	printf("}\n");
}

static void gen_scope(Scope *scope)
{
	for(Node *symbol = scope->symbols; symbol; symbol = symbol->next) {
		if(symbol->kind == ND_VARDECL) {
			gen_vardecl(symbol);
		}
	}
	
	for(Node *symbol = scope->symbols; symbol; symbol = symbol->next) {
		if(symbol->kind == ND_FUNCDECL) {
			gen_funcdecl(symbol);
		}
	}
}

static void gen_block(Node *node)
{
	if(node == 0) {
		return;
	}
	
	gen_scope(node->scope);
}

void generate(Node *ast)
{
	gen_block(ast);
}
