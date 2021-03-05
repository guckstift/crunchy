#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef struct Node Node;
typedef struct Scope Scope;

typedef enum {
	TK_INT,
	TK_IDENT,
	TK_PUNCT,
	TK_END,
} TokenKind;

typedef struct {
	TokenKind kind;
	char *name;
	size_t line;
	size_t length;
	char *text;
	void *next;
} Token;

typedef struct {
	Token *first;
	Token *last;
} Tokens;

typedef enum {
	ND_PRIM,
	ND_CHAIN,
	ND_ASSIGN,
	ND_VARDECL,
	ND_PRIMTYPE,
	ND_PRINT,
	ND_BLOCK,
	ND_FUNCDECL,
	ND_CALL,
} NodeKind;

struct Node {
	NodeKind kind;
	char *name;
	Token *token;
	int tier;
	Node *expr;
	Node *next;
	Node *type;
	Node *stmt;
	Node *body;
	Scope *scope;
	int resolved;
	int resolving;
	int isconst;
};

struct Scope {
	Node *symbols;
	Node *last;
	void *parent;
};

typedef struct {
	char *filename;
	char *source;
	Tokens *tokens;
	Node *ast;
} Unit;

// ast.c

Scope *create_scope(Scope *parent);
Node *lookup_symbol(Scope *scope, Token *ident);
Node *lookup_symbol_rec(Scope *scope, Token *ident);
int declare_symbol(Scope *scope, Node *symbol);
Node *create_prim(Token *token);
Node *create_chain(Node *head, Token *op, int tier, Node *next);
Node *create_assign(Token *ident, Node *expr);
Node *create_vardecl(Token *ident, Node *type, Node *init);
Node *create_primtype(char *name);
Node *create_block(Node *stmt, Scope *scope);
Node *create_print(Node *expr);
Node *create_funcdecl(Token *ident, Node *body);
Node *create_call(Token *ident);

// dump.c

void dump_tokens(Tokens *tokens);
void dump_ast(Node *ast);

// generate.c

void generate(Node *ast);

// lex.c

Tokens *lex(char *source);

// parse.c

Node *parse(Tokens *tokens);

// resolve.c

void resolve(Node *ast);

// utils.c

char *read_file(char *filename);
char *clone_substring(char *start, size_t length);
size_t match_substring(char *start, char *substring);
