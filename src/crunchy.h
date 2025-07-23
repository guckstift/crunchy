#include <stdio.h>
#include <stdint.h>

#define KEYWORDS \
	_(bool) \
	_(false) \
	_(if) \
	_(int) \
	_(print) \
	_(string) \
	_(true) \
	_(var) \

#define PUNCTS \
	_('=', EQUALS) \
	_(';', SEMICOLON) \
	_(':', COLON) \
	_('{', LCURLY) \
	_('}', RCURLY) \
	_('+', PLUS) \

typedef enum : uint8_t {
	TK_BOF,
	TK_EOF,

	TK_INT,
	TK_IDENT,
	TK_STRING,

	#define _(a) KW_ ## a,
	KEYWORDS
	#undef _

	#define _(a, b) PT_ ## b,
	PUNCTS
	#undef _

	TYPE_KIND_START,

	TY_INT,
	TY_BOOL,
	TY_STRING,

	EXPR_KIND_START,

	EX_INT,
	EX_BOOL,
	EX_STRING,
	EX_VAR,
	EX_CAST,
	EX_BINOP,

	STMT_KIND_START,

	ST_VARDECL,
	ST_PRINT,
	ST_ASSIGN,
	ST_IF,
} Kind;

typedef struct {
	Kind kind;
	char *start;
	int64_t length;
	int64_t line;

	union {
		int64_t ival;
		char *chars;
	};

	int64_t str_length;
} Token;

typedef struct {
	Kind kind;
} Type;

typedef struct {
	Kind kind;
	Token *start;
	uint8_t is_lvalue : 1;
	Type *type;

	union {
		int64_t ival; // int, bool
		Token *ident; // var
		void *subexpr; // cast
		void *left; // binop
		char *chars; // string
	};

	union {
		struct Stmt *decl; // var
		void *right; // binop
		int64_t length; // string
	};

	Token *op; // binop
} Expr;

typedef struct Stmt {
	Kind kind;
	void *next;
	struct Block *parent_block;
	Token *start;
	Token *end;

	union {
		Token *ident; // vardecl
		Expr *target; // assign
		Expr *cond; // if
	};

	union {
		Type *type; // vardecl
		Expr *value; // assign, print
		void *body; // if
	};

	Expr *init; // vardecl
	void *next_decl; // vardecl
} Stmt;

typedef struct Block {
	void *parent;
	int64_t id;
	Stmt *stmts;
	Stmt *decls;
	Stmt *last_decl;
	int64_t num_gc_decls;
} Block;

// main
void error(char *msg);

// print
void set_print_file(FILE *new_fs);
void print(char *msg, ...);
void print_token_list(Token *tokens);
void print_block(Block *block);

// lex
int64_t lex(char *src, Token **tokens_out);

// parse
Type *new_type(Kind kind);
Expr *new_expr(Kind kind, Token *start, uint8_t is_lvalue);
Stmt *new_stmt(Kind kind, Block *parent, Token *start, Token *end);
Block *parse(Token *tokens);

// analyse
void analyse(Block *block);

// generate
void generate(Block *block, char *output_file);