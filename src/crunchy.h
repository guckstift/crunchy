#include <stdint.h>

typedef enum : uint8_t {
	TK_INVALID,
	TK_EOF,

	TK_INT,
	TK_IDENT,

	KW_var,
	KW_int,

	PT_EQUALS,
	PT_SEMICOLON,
	PT_COLON,
} TokenKind;

typedef struct {
	TokenKind kind;
	char *start;
	char *end;
	int64_t ival;
} Token;

typedef enum : uint8_t {
	TY_INVALID,

	TY_INT,
} TypeKind;

typedef struct {
	TypeKind kind;
} Type;

typedef enum : uint8_t {
	EX_INVALID,

	EX_INT,
} ExprKind;

typedef struct {
	ExprKind kind;
	Type *type;
	int64_t ival;
} Expr;

typedef enum : uint8_t {
	ST_INVALID,

	ST_VARDECL,
} StmtKind;

typedef struct {
	StmtKind kind;
	void *next;
	Token *ident;
	Type *type;
	Expr *init;
} Stmt;

void error(char *msg);
void print(Stmt *stmts);
int64_t lex(char *src, Token **tokens_out);
Stmt *parse(Token *tokens);
void analyse(Stmt *stmts);
void generate(Stmt *stmts, char *output_file);