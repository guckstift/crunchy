typedef enum {
	// tokens
	INTEGER,
	IDENT,
	PUNCT,
	END,
	// expressions
	PRIM,
	CHAIN,
	// types
	PRIMTYPE,
	// statements
	ASSIGN,
	VARDECL,
	FUNCDECL,
	CALL,
	PRINT,
} Kind;

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

typedef struct {
	Kind kind;
	char *primtype;
} Type;

typedef struct Expr {
	Kind kind;
	Token *prim;
	struct Expr *left;
	struct Expr *right;
	Token *op;
	Type *type;
	int isconst;
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
	Expr *expr;
	Type *type;
	struct Block *body;
	State state;
} Stmt;

typedef struct Symbol {
	struct Symbol *next;
	Stmt *decl;
	Token *ident;
} Symbol;

typedef struct Scope {
	struct Scope *parent;
	Symbol *first;
	Symbol *last;
	size_t count;
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
	struct Unit *next;
	char *source;
	size_t length;
	TokenList *tokens;
	Block *ast;
} Unit;

typedef struct {
	Unit *first;
	Unit *last;
	size_t count;
} UnitList;

typedef struct Project {
	UnitList *units;
	Unit *main;
	char *exepath;
} Project;
