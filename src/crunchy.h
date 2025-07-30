#include <stdio.h>
#include <stdint.h>

#define KEYWORDS \
	_(bool) \
	_(else) \
	_(false) \
	_(function) \
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
	_('(', LPAREN) \
	_(')', RPAREN) \
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

	TY_VOID,
	TY_INT,
	TY_BOOL,
	TY_STRING,
	TY_FUNC,

	EXPR_KIND_START,

	EX_NOOPFUNC,
	EX_INT,
	EX_BOOL,
	EX_STRING,
	EX_VAR,
	EX_CAST,
	EX_BINOP,
	EX_CALL,

	STMT_KIND_START,

	ST_VARDECL,
	ST_FUNCDECL,
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
	void *next;
	Type *type;
	struct Block *parent_block;
	int64_t id;
} Temp;

typedef struct {
	Kind kind;
	Token *start;
	uint8_t is_lvalue : 1;
	Temp *temp;
	Type *type;

	union {
		int64_t ival; // int, bool
		Token *ident; // var
		void *subexpr; // cast
		void *left; // binop
		void *callee; // call
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
		Token *ident; // vardecl, funcdecl
		Expr *target; // assign
		Expr *cond; // if
	};

	union {
		Expr *init; // vardecl
		Expr *value; // assign, print
		void *body; // if, funcdecl
	};

	union {
		Type *type; // vardecl, funcdecl
		void *else_body; // if
	};

	void *next_decl; // vardecl, funcdecl
} Stmt;

typedef struct Block {
	void *parent;
	int64_t id;
	Stmt *stmts;
	Stmt *decls;
	Stmt *last_decl;
	int64_t num_gc_decls;
	Temp *temps;
	Temp *last_temp;
} Block;

typedef void (*EscapeMod)(va_list);

// main
void error(char *msg);
void error_at(Token *at, char *msg);

// print
void set_print_file(FILE *new_fs);
void set_escape_mod(char chr, EscapeMod mod);
int64_t print(char *msg, ...);
void print_token_list(Token *tokens);
int64_t print_block(Block *block);

// lex
int64_t lex(char *src, Token **tokens_out);

// parse
Type *new_type(Kind kind);
Expr *new_expr(Kind kind, Token *start, uint8_t is_lvalue);
Stmt *new_stmt(Kind kind, Block *parent, Token *start, Token *end);
int declare_in(Stmt *decl, Block *block);
Block *parse(Token *tokens);

// analyse
void analyse(Block *block);

// generate
void generate(Block *block, char *output_file);