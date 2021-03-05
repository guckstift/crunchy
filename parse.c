#include "crunchy.h"

typedef struct {
	Token *token;
	size_t line;
	Scope *scope;
} ParseState;

typedef Node *(*ParseNodeFunc)();

static ParseState *state;

static Node *parse_funcdecl();

static void error(char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "error at line %lu: ", state->line + 1);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	exit(1);
}

static Token *consume()
{
	Token *token = state->token;
	state->token = token->next;
	state->line = state->token->line;
	return token;
}

static void reset(Token *token)
{
	state->token = token;
	state->line = token->line;
}

static Token *parse_kind(TokenKind kind)
{
	Token *token = state->token;
	
	if(token && token->kind == kind) {
		return consume();
	}
	
	return 0;
}

static Token *parse_text(char *text)
{
	Token *token = state->token;
	
	if(token && strcmp(token->text, text) == 0) {
		return consume();
	}
	
	return 0;
}

static Node *parse_prim()
{
	Token *token = parse_kind(TK_INT);
	
	if(token) {
		return create_prim(token);
	}
	
	token = parse_kind(TK_IDENT);
	
	if(token) {
		return create_prim(token);
	}
	
	return 0;
}

static Token *parse_op(char *ops)
{
	if(state->token == 0) {
		return 0;
	}
	
	while(*ops) {
		if(match_substring(ops, state->token->text)) {
			return parse_kind(TK_PUNCT);
		}
		
		while(*ops != ' ') {
			ops ++;
		}
		
		ops ++;
	}
	
	return 0;
}

static Node *parse_chain(int tier)
{
	static char *optable[] = {
		"+ - ",
		"* / ",
		0
	};
	
	char *ops = optable[tier];
	
	if(ops == 0) {
		return parse_prim();
	}
	
	Node *head = parse_chain(tier + 1);
	Node *last = head;
	Node *prev = 0;
	
	while(1) {
		Token *op = parse_op(ops);
		
		if(op == 0) {
			break;
		}
		
		Node *next = parse_chain(tier + 1);
		
		if(next == 0) {
			error("expected expression after '%s'", op->text);
		}
		
		Node *wrapped = create_chain(last, op, tier, next);
		
		if(head == last) {
			head = wrapped;
		}
		
		if(prev) {
			prev->next = wrapped;
		}
		
		prev = wrapped;
		last = next;
	}
	
	return head;
}

static Node *parse_expr()
{
	return parse_chain(0);
}

static Node *parse_assign()
{
	Token *start = state->token;
	Token *ident = parse_kind(TK_IDENT);
	
	if(ident == 0) {
		return 0;
	}
	
	if(parse_text("=") == 0) {
		reset(start);
		return 0;
	}
	
	Node *expr = parse_expr();
	
	if(expr == 0) {
		error("expected expression after '='");
	}
	
	return create_assign(ident, expr);
}

static Node *parse_primtype()
{
	Token *token = parse_text("int");
	
	if(token) {
		return create_primtype(token->text);
	}
	
	return 0;
}

static Node *parse_type()
{
	return parse_primtype();
}

static Node *parse_vardecl()
{
	Token *start = state->token;
	Token *ident = parse_kind(TK_IDENT);
	
	if(ident == 0) {
		return 0;
	}
	
	Node *type = 0;
	Node *init = 0;
	
	if(parse_text(":")) {
		if(lookup_symbol(state->scope, ident)) {
			error("'%s' is already declared", ident->text);
		}
		
		type = parse_type();
		
		if(type == 0) {
			error("expected type after ':'");
		}
		
		if(parse_text("=")) {
			init = parse_expr();
			
			if(init == 0) {
				error("expected initializer after '='");
			}
		}
	}
	else if(parse_text(":=")) {
		if(lookup_symbol(state->scope, ident)) {
			error("'%s' is already declared", ident->text);
		}
		
		init = parse_expr();
		
		if(init == 0) {
			error("expected initializer after ':='");
		}
	}
	else {
		reset(start);
		return 0;
	}
	
	Node *vardecl = create_vardecl(ident, type, init);
	declare_symbol(state->scope, vardecl);
	return vardecl;
}

static Node *parse_print()
{
	if(parse_text("print") == 0) {
		return 0;
	}
	
	Node *expr = parse_expr();
	
	if(expr == 0) {
		error("expected expression after 'print'");
	}
	
	return create_print(expr);
}

static Node *parse_call()
{
	Token *start = state->token;
	Token *ident = parse_kind(TK_IDENT);
	
	if(ident == 0) {
		return 0;
	}
	
	if(parse_text("(") == 0) {
		reset(start);
		return 0;
	}
	
	if(parse_text(")") == 0) {
		error("expected ')' after '('");
	}
	
	return create_call(ident);
}

static Node *parse_stmt()
{
	ParseNodeFunc funcs[] = {
		parse_funcdecl, parse_vardecl, parse_print, parse_call, parse_assign,
		0
	};
	
	for(int i=0; funcs[i]; i++) {
		ParseNodeFunc func = funcs[i];
		Node *node = func();
		
		if(node) {
			return node;
		}
	}
}

static Node *parse_block()
{
	Node *head = 0;
	Node *last = 0;
	state->scope = create_scope(state->scope);
	
	while(1) {
		Node *stmt = parse_stmt();
		
		if(stmt == 0) {
			break;
		}
		
		Node *block = create_block(stmt, state->scope);
		
		if(head == 0) {
			head = block;
		}
		
		if(last) {
			last->next = block;
		}
		
		last = block;
	}
	
	state->scope = state->scope->parent;
	return head;
}

static Node *parse_funcdecl()
{
	if(parse_text("func") == 0) {
		return 0;
	}
	
	if(state->scope->parent) {
		error("functions can only be declared globally");
	}
	
	Token *ident = parse_kind(TK_IDENT);
	
	if(ident == 0) {
		error("expected function name after 'func'");
	}
	
	if(lookup_symbol(state->scope, ident)) {
		error("'%s' is already declared", ident->text);
	}
	
	if(parse_text("(") == 0) {
		error("expected '(' after function name");
	}
	
	if(parse_text(")") == 0) {
		error("expected ')' after '('");
	}
	
	if(parse_text("{") == 0) {
		error("expected '{' after ')'");
	}
	
	Node *body = parse_block();
	
	if(parse_text("}") == 0) {
		error("expected '}' after function statements");
	}
	
	Node *funcdecl = create_funcdecl(ident, body);
	declare_symbol(state->scope, funcdecl);
	return funcdecl;
}

Node *parse(Tokens *tokens)
{
	state = calloc(1, sizeof(ParseState));
	state->token = tokens->first;
	state->line = 0;
	Node *ast = parse_block();
	
	if(state->token->kind != TK_END) {
		error("unexpected end of input");
	}
	
	free(state);
	return ast;
}
