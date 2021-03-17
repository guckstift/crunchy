#include <stddef.h>

typedef enum {
	// tokens
	INTEGER,
	IDENT,
	STRING,
	PUNCT,
	END,
	// expressions
	PRIM,
	PTR,
	CALL,
	DEREF,
	CHAIN,
	// types
	PRIMTYPE,
	PTRTYPE,
	// statements
	ASSIGN,
	VARDECL,
	FUNCDECL,
	CALLSTMT,
	PRINT,
	RETURN,
	IMPORT,
	IFSTMT,
	WHILESTMT,
} Kind;

typedef enum {
	U8, U16, U32, U64,
	I8, I16, I32, I64,
} PrimType;

typedef struct Token {
	struct Token *next;
	Kind kind;
	size_t line;
	size_t pos;
	size_t val;
	char *text;
} Token;

typedef struct {
	Token *first;
	Token *last;
	size_t count;
} TokenList;

typedef struct Type {
	Kind kind;
	PrimType primtype;
	struct Type *child;
} Type;

typedef struct Expr {
	Kind kind;
	Token *prim;
	Token *ident;
	struct Expr *left;
	struct Expr *right;
	struct Expr *child;
	Token *op;
	Type *type;
	int isconst;
	int iscallstmt;
} Expr;

typedef enum {
	UNRESOLVED,
	RESOLVING,
	RESOLVED,
} State;

typedef struct Stmt {
	Kind kind;
	struct Stmt *next;
	Token *ident;
	Token *string;
	Expr *expr;
	Type *type;
	struct Block *body;
	State state;
	int exported;
	size_t exporthash;
	struct Unit *unit;
} Stmt;

typedef struct Symbol {
	struct Symbol *next;
	Stmt *decl;
	Token *ident;
} Symbol;

typedef struct Scope {
	struct Scope *parent;
	Stmt *funchost;
	Symbol *first;
	Symbol *last;
	size_t count;
	Symbol *first_import;
	Symbol *last_import;
	size_t import_count;
} Scope;

typedef struct Block {
	Stmt *first;
	Stmt *last;
	size_t count;
	Scope *scope;
} Block;

typedef struct Unit {
	char *filepath;
	char *abspath;
	char *filename;
	char *cpath;
	char *opath;
	size_t hash;
	struct Unit *next;
	char *source;
	size_t length;
	TokenList *tokens;
	Block *ast;
} Unit;

typedef struct UnitList {
	Unit *first;
	Unit *last;
	size_t count;
} UnitList;

typedef struct Project {
	UnitList *units;
	Unit *main;
	char *exepath;
} Project;
