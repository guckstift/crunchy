#include <stdint.h>

#define KEYWORDS \
	_(bool) \
	_(false) \
	_(int) \
	_(true) \
	_(var) \

#define PUNCTS \
	_('=', EQUALS) \
	_(';', SEMICOLON) \
	_(':', COLON) \

typedef enum : uint8_t {
	TK_INVALID,
	TK_EOF,

	TK_INT,
	TK_IDENT,

	#define _(a) KW_ ## a,
	KEYWORDS
	#undef _

	#define _(a, b) PT_ ## b,
	PUNCTS
	#undef _
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
	TY_BOOL,
} TypeKind;

typedef struct {
	TypeKind kind;
} Type;

typedef enum : uint8_t {
	EX_INVALID,

	EX_INT,
	EX_BOOL,
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
void print_tokens(Token *tokens);
void print_stmts(Stmt *stmts);
int64_t lex(char *src, Token **tokens_out);
Stmt *parse(Token *tokens);
void analyse(Stmt *stmts);
void generate(Stmt *stmts, char *output_file);