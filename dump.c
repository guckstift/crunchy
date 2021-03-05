#include <stdio.h>
#include "crunchy.h"

static void indent(int level)
{
	for(int i=0; i<level; i++) {
		printf("  ");
	}
}

static void dump_token(Token *token, int level, char *prepend)
{
	indent(level);
	printf("%s%s '%s'\n", prepend, token->name, token->text);
}

void dump_tokens(Tokens *tokens)
{
	for(Token *token = tokens->first; token; token = token->next) {
		dump_token(token, 0, "");
	}
}

static void dump_scope(Scope *scope, int level, char *prepend)
{
	indent(level);
	printf("%s", prepend);
	
	for(Node *symbol = scope->symbols; symbol; symbol = symbol->next) {
		printf("%s ", symbol->token->text);
	}
	
	printf("\n");
}

static void dump_node(Node *node, int level, char *prepend)
{
	if(node == 0) {
		indent(level);
		printf("%s<null>\n", prepend);
		return;
	}
	
	if(node->kind == ND_PRIM) {
		dump_token(node->token, level, prepend);
		return;
	}
	
	indent(level);
	printf("%s%s\n", prepend, node->name);
	
	if(node->kind == ND_CHAIN) {
		dump_node(node->expr, level + 1, "operand: ");
		dump_token(node->token, level + 1, "op: ");
		Node *next = node->next;
		
		while(next->kind == ND_CHAIN && next->tier == node->tier) {
			dump_node(next->expr, level + 1, "operand: ");
			dump_token(next->token, level + 1, "op: ");
			next = next->next;
		}
		
		dump_node(next, level + 1, "operand: ");
	}
	else if(node->kind == ND_ASSIGN) {
		dump_token(node->token, level + 1, "ident: ");
		dump_node(node->expr, level + 1, "expr: ");
	}
	else if(node->kind == ND_VARDECL) {
		dump_token(node->token, level + 1, "ident: ");
		dump_node(node->type, level + 1, "type: ");
		dump_node(node->expr, level + 1, "init: ");
	}
	else if(node->kind == ND_BLOCK) {
		dump_scope(node->scope, level + 1, "scope: ");
		
		for(Node *block = node; block; block = block->next) {
			dump_node(block->stmt, level + 1, "");
		}
	}
	else if(node->kind == ND_PRINT) {
		dump_node(node->expr, level + 1, "expr: ");
	}
	else if(node->kind == ND_FUNCDECL) {
		dump_token(node->token, level + 1, "ident: ");
		dump_node(node->body, level + 1, "body: ");
	}
	else if(node->kind == ND_CALL) {
		dump_token(node->token, level + 1, "ident: ");
	}
}

void dump_ast(Node *ast)
{
	dump_node(ast, 0, "ast: ");
}
