#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>

typedef enum {
	// tokens
	INTEGER,
	FLOAT,
	IDENT,
	STRING,
	PUNCT,
	END,
	// expressions
	PRIM,
	UNARY,
	SUBSCRIPT,
	ARRAY,
	PTR,
	CALL,
	DEREF,
	ADDRESS,
	MEMBER,
	CHAIN,
	// types
	PRIMTYPE,
	PTRTYPE,
	ARRAYTYPE,
	NAMEDTYPE,
	STRUCTTYPE,
	// statements
	ASSIGN,
	VARDECL,
	FUNCDECL,
	STRUCTDECL,
	CALLSTMT,
	PRINT,
	RETURN,
	IMPORT,
	IFSTMT,
	WHILESTMT,
	BREAK,
	CONTINUE,
} Kind;

typedef enum {
	U8, U16, U32, U64,
	I8, I16, I32, I64,
	F32, F64,
} PrimType;

typedef struct Token {
	Kind kind;
	size_t line;
	size_t pos;
	size_t val;
	double fval;
	char *text;
	size_t length;
} Token;

typedef struct Type {
	Kind kind;
	PrimType primtype;
	struct Type *child;
	Token *ident;
	struct Stmt *typedecl;
	size_t count;
} Type;

typedef enum {
	LOGOR,
	LOGAND,
	RELATIONAL,
	ADDITIVE,
	MULTIPLICATIVE,
} OpTier;

typedef struct Expr {
	Kind kind;
	Token *prim;
	Token *ident;
	struct Expr *left;
	struct Expr *right;
	struct Expr *child;
	struct Expr *next;
	size_t length;
	Token *op;
	OpTier tier;
	Type *type;
	int isconst;
	int iscallstmt;
	int islvalue;
	int64_t iconst;
} Expr;

typedef enum {
	UNRESOLVED,
	RESOLVING,
	RESOLVED,
} State;

typedef struct Stmt {
	Kind kind;
	struct Stmt *next;
	struct Stmt *param;
	size_t param_count;
	int isparam;
	Token *ident;
	Token *string;
	Token *op;
	Expr *target;
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
	Token *tokens;
	size_t tcount;
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