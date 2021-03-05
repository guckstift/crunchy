#include "crunchy.h"

typedef struct {
	Scope *scope;
	size_t line;
} ResolveState;

static ResolveState *state;

void resolve_funcdecl(Node *node);
void resolve_expr(Node *node);
void resolve_block(Node *node);

static void error(char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "error at line %lu: ", state->line + 1);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	exit(1);
}

static Node *lookup_ident(Token *ident)
{
	return lookup_symbol_rec(state->scope, ident);
}

Node *check_var_ident(Token *ident)
{
	state->line = ident->line;
	Node *symbol = lookup_ident(ident);
	
	if(symbol == 0) {
		error("'%s' is not declared", ident->text);
	}
	else if(symbol->kind == ND_VARDECL) {
		if(symbol->resolved == 0) {
			error("'%s' is used before initialization", ident->text);
		}
	}
	else {
		error("'%s' is not a variable", ident->text);
	}
	
	return symbol;
}

void resolve_prim(Node *node)
{
	if(node == 0 || node->resolved || node->resolving) {
		return;
	}
	
	node->resolving = 1;
	Token *prim = node->token;
	
	if(prim->kind == TK_IDENT) {
		Node *symbol = check_var_ident(prim);
		node->type = symbol->type;
	}
	else if(prim->kind == TK_INT) {
		node->type = create_primtype("int");
		node->isconst = 1;
	}
	
	node->resolved = 1;
}

void resolve_chain(Node *node)
{
	if(node == 0 || node->resolved || node->resolving) {
		return;
	}
	
	node->resolving = 1;
	node->isconst = 1;
	
	for(Node *cur = node; cur; cur = cur->next) {
		if(cur->kind == ND_CHAIN && cur->tier == node->tier) {
			resolve_expr(cur->expr);
			cur->type = cur->expr->type;
		}
		else {
			resolve_expr(cur);
		}
		
		node->isconst = node->isconst && cur->isconst;
	}
	
	node->resolved = 1;
}

void resolve_expr(Node *node)
{
	if(node == 0) {
		return;
	}
	
	if(node->kind == ND_PRIM) {
		resolve_prim(node);
	}
	else if(node->kind == ND_CHAIN) {
		resolve_chain(node);
	}
}

void resolve_assign(Node *node)
{
	if(node == 0 || node->resolved || node->resolving) {
		return;
	}
	
	node->resolving = 1;
	Token *ident = node->token;
	check_var_ident(ident);
	resolve_expr(node->expr);
	node->resolved = 1;
}

void resolve_call(Node *node)
{
	if(node == 0 || node->resolved || node->resolving) {
		return;
	}
	
	node->resolving = 1;
	Token *ident = node->token;
	state->line = ident->line;
	Node *symbol = lookup_ident(ident);
	
	if(symbol == 0) {
		error("'%s' is not declared", ident->text);
	}
	
	if(symbol->kind != ND_FUNCDECL) {
		error("'%s' is not a function", ident->text);
	}
	
	resolve_funcdecl(symbol);
	node->resolved = 1;
}

void resolve_print(Node *node)
{
	if(node == 0 || node->resolved || node->resolving) {
		return;
	}
	
	node->resolving = 1;
	resolve_expr(node->expr);
	node->resolved = 1;
}

void resolve_vardecl(Node *node)
{
	if(node == 0 || node->resolved || node->resolving) {
		return;
	}
	
	node->resolving = 1;
	resolve_expr(node->expr);
	
	if(node->type == 0) {
		node->type = node->expr->type;
	}
	
	node->resolved = 1;
}

void resolve_funcdecl(Node *node)
{
	if(node == 0 || node->resolved || node->resolving) {
		return;
	}
	
	node->resolving = 1;
	resolve_block(node->body);
	node->resolved = 1;
}

void resolve_stmt(Node *node)
{
	if(node == 0) {
		return;
	}
	
	if(node->kind == ND_ASSIGN) {
		resolve_assign(node);
	}
	else if(node->kind == ND_PRINT) {
		resolve_print(node);
	}
	else if(node->kind == ND_VARDECL) {
		resolve_vardecl(node);
	}
	else if(node->kind == ND_FUNCDECL) {
		resolve_funcdecl(node);
	}
	else if(node->kind == ND_CALL) {
		resolve_call(node);
	}
}

void resolve_block(Node *node)
{
	if(node == 0 || node->resolved || node->resolving) {
		return;
	}
	
	node->resolving = 1;
	Scope *oldscope = state->scope;
	state->scope = node->scope;
	
	for(Node *cur = node; cur; cur = cur->next) {
		resolve_stmt(cur->stmt);
	}
	
	state->scope = oldscope;
	node->resolved = 1;
}

void resolve(Node *ast)
{
	state = calloc(1, sizeof(ResolveState));
	resolve_block(ast);
	free(state);
}
