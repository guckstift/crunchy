#include "crunchy.h"

Scope *create_scope(Scope *parent)
{
	Scope *scope = calloc(1, sizeof(Scope));
	scope->parent = parent;
	return scope;
}

Node *lookup_symbol(Scope *scope, Token *ident)
{
	for(Node *symbol = scope->symbols; symbol; symbol = symbol->next) {
		Token *symbol_ident = symbol->token;
		
		if(strcmp(symbol_ident->text, ident->text) == 0) {
			return symbol;
		}
	}
	
	return 0;
}

Node *lookup_symbol_rec(Scope *scope, Token *ident)
{
	Node *symbol = lookup_symbol(scope, ident);
	
	if(symbol) {
		return symbol;
	}
	
	if(scope->parent) {
		return lookup_symbol_rec(scope->parent, ident);
	}
	
	return 0;
}

int declare_symbol(Scope *scope, Node *symbol)
{
	if(lookup_symbol(scope, symbol->token)) {
		return 0;
	}
	
	if(scope->symbols == 0) {
		scope->symbols = symbol;
	}
	else {
		scope->last->next = symbol;
	}
	
	scope->last = symbol;
	return 1;
}

Node *create_prim(Token *token)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_PRIM;
	node->name = "PRIM";
	node->token = token;
	return node;
}

Node *create_chain(Node *head, Token *op, int tier, Node *next)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_CHAIN;
	node->name = "CHAIN";
	node->token = op;
	node->tier = tier;
	node->expr = head;
	node->next = next;
	return node;
}

Node *create_assign(Token *ident, Node *expr)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_ASSIGN;
	node->name = "ASSIGN";
	node->token = ident;
	node->expr = expr;
	return node;
}

Node *create_vardecl(Token *ident, Node *type, Node *init)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_VARDECL;
	node->name = "VARDECL";
	node->token = ident;
	node->expr = init;
	node->type = type;
	return node;
}

Node *create_primtype(char *name)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_PRIMTYPE;
	node->name = name;
	return node;
}

Node *create_block(Node *stmt, Scope *scope)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_BLOCK;
	node->name = "BLOCK";
	node->stmt = stmt;
	node->scope = scope;
	return node;
}

Node *create_print(Node *expr)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_PRINT;
	node->name = "PRINT";
	node->expr = expr;
	return node;
}

Node *create_funcdecl(Token *ident, Node *body)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_FUNCDECL;
	node->name = "FUNCDECL";
	node->token = ident;
	node->body = body;
	return node;
}

Node *create_call(Token *ident)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_CALL;
	node->name = "CALL";
	node->token = ident;
	return node;
}
