char *optable[] = {
	"+ - ",
	"* / ",
	0,
};

Scope *scope = 0;

Block *parse_block();

Symbol *lookup(Token *ident)
{
	for(Symbol *symbol = scope->first; symbol; symbol = symbol->next) {
		if(strcmp(symbol->ident->text, ident->text) == 0) {
			return symbol;
		}
	}
	
	return 0;
}

Symbol *lookup_rec(Token *ident)
{
	Symbol *symbol = lookup(ident);
	
	if(symbol) {
		return symbol;
	}
	
	if(scope->parent) {
		Scope *backup = scope;
		scope = scope->parent;
		symbol = lookup_rec(ident);
		scope = backup;
	}
	
	return symbol;
}

void declare(Stmt *decl)
{
	Symbol *symbol = create(Symbol);
	symbol->decl = decl;
	symbol->ident = decl->ident;
	
	if(scope->count) {
		scope->last->next = symbol;
		scope->last = symbol;
	}
	else {
		scope->first = symbol;
		scope->last = symbol;
	}
	
	scope->count ++;
}

Token *next_token()
{
	Token *temp = token;
	token = token->next;
	line = token->line;
	pos = token->pos;
	return temp;
}

void seek_token(Token *tok)
{
	token = tok;
	line = token->line;
	pos = token->pos;
}

int is_kind(Kind kind)
{
	return token->kind == kind;
}

int is_keyword(char *key)
{
	return is_kind(IDENT) && strcmp(token->text, key) == 0;
}

int is_punct(char *punct)
{
	return is_kind(PUNCT) && strcmp(token->text, punct) == 0;
}

Token *parse_kind(Kind kind)
{
	if(is_kind(kind)) {
		return next_token();
	}
	
	return 0;
}

Token *parse_keyword(char *key)
{
	if(is_keyword(key)) {
		return next_token();
	}
	
	return 0;
}

Token *parse_punct(char *punct)
{
	if(is_punct(punct)) {
		return next_token();
	}
	
	return 0;
}

Expr *parse_prim()
{
	if(is_kind(INTEGER) || is_kind(IDENT)) {
		Expr *prim = create(Expr);
		prim->kind = PRIM;
		prim->prim = next_token();
		prim->isconst = prim->prim->kind != IDENT;
		return prim;
	}
	
	return 0;
}

Token *parse_op(char *ops)
{
	if(!is_kind(PUNCT))
		return 0;
	
	while(*ops) {
		if(match_strpart(ops, token->text))
			return next_token();
		
		while(*ops != ' ')
			ops ++;
		
		ops ++;
	}
	
	return 0;
}

Expr *parse_chain(int tier)
{
	char *ops = optable[tier];
	
	if(ops == 0)
		return parse_prim();
	
	Expr *left = parse_chain(tier + 1);
	
	if(left == 0)
		return 0;
	
	while(1) {
		Token *op = parse_op(ops);
		
		if(op == 0)
			return left;
		
		Expr *right = parse_chain(tier + 1);
		
		if(right == 0)
			error("right side expected after binary operator '%s'", op->text);
		
		Expr *chain = create(Expr);
		chain->kind = CHAIN;
		chain->left = left;
		chain->right = right;
		chain->op = op;
		chain->isconst = left->isconst && right->isconst;
		left = chain;
	}
}

Expr *parse_expr()
{
	return parse_chain(0);
}

Type *parse_primtype()
{
	if(is_keyword("int")) {
		Type *type = create(Type);
		type->kind = PRIMTYPE;
		type->primtype = next_token()->text;
		return type;
	}
	
	return 0;
}

Type *parse_type()
{
	return parse_primtype();
}

Stmt *parse_funcdecl()
{
	if(parse_keyword("func") == 0)
		return 0;
	
	if(scope->parent)
		error("functions can only be declared in global scope");
	
	Token *ident = parse_kind(IDENT);
	
	if(ident == 0)
		error("expected function name after 'func'");
	
	if(lookup(ident))
		error("'%s' is already declared", ident->text);
	
	if(parse_punct("(") == 0)
		error("expected '(' after function name");
	
	if(parse_punct(")") == 0)
		error("expected ')' after '('");
	
	if(parse_punct("{") == 0)
		error("expected '{' after ')'");
	
	Block *body = parse_block();
	
	if(parse_punct("}") == 0)
		error("expected '}' after function statements");
	
	Stmt *funcdecl = create(Stmt);
	funcdecl->kind = FUNCDECL;
	funcdecl->ident = ident;
	funcdecl->body = body;
	declare(funcdecl);
	return funcdecl;
}

Stmt *parse_assign()
{
	Token *start = token;
	Token *ident = parse_kind(IDENT);
	
	if(ident == 0)
		return 0;
	
	if(parse_punct("=") == 0) {
		seek_token(start);
		return 0;
	}
	
	Expr *expr = parse_expr();
	
	if(expr == 0)
		error("expected expression after '='");
	
	Stmt *assign = create(Stmt);
	assign->kind = ASSIGN;
	assign->ident = ident;
	assign->expr = expr;
	return assign;
}

Stmt *parse_vardecl()
{
	Token *start = token;
	Token *ident = parse_kind(IDENT);
	Type *type = 0;
	Expr *expr = 0;
	
	if(ident == 0)
		return 0;
	
	if(parse_punct(":")) {
		if(lookup(ident))
			error("'%s' is already declared", ident->text);
		
		type = parse_type();
		
		if(type == 0)
			error("expected type after ':'");
		
		if(parse_punct("=")) {
			expr = parse_expr();
			
			if(expr == 0)
				error("expected initializer after '='");
		}
	}
	else if(parse_punct(":=")) {
		if(lookup(ident))
			error("'%s' is already declared", ident->text);
		
		expr = parse_expr();
		
		if(expr == 0)
			error("expected initializer after ':='");
	}
	else {
		seek_token(start);
		return 0;
	}
	
	Stmt *vardecl = create(Stmt);
	vardecl->kind = VARDECL;
	vardecl->ident = ident;
	vardecl->type = type;
	vardecl->expr = expr;
	declare(vardecl);
	return vardecl;
}

Stmt *parse_call()
{
	Token *start = token;
	Token *ident = parse_kind(IDENT);
	
	if(ident == 0)
		return 0;
	
	if(parse_punct("(") == 0) {
		seek_token(start);
		return 0;
	}
	
	if(parse_punct(")") == 0)
		error("expected ')' after '('");
	
	Stmt *call = create(Stmt);
	call->kind = CALL;
	call->ident = ident;
	return call;
}

Stmt *parse_print()
{
	if(parse_keyword("print") == 0)
		return 0;
	
	Expr *expr = parse_expr();
	
	if(expr == 0)
		error("expected expression after 'print'");
	
	Stmt *print = create(Stmt);
	print->kind = PRINT;
	print->expr = expr;
	return print;
}

Stmt *parse_stmt()
{
	Stmt *stmt;
	
	(stmt = parse_funcdecl()) ||
	(stmt = parse_print()) ||
	(stmt = parse_assign()) ||
	(stmt = parse_vardecl()) ||
	(stmt = parse_call()) ||
	0;
	
	return stmt;
}

Block *parse_block()
{
	Stmt *first = 0;
	Stmt *last = 0;
	size_t count = 0;
	Scope *blockscope = create(Scope);
	blockscope->parent = scope;
	scope = blockscope;
	
	while(1) {
		Stmt *stmt = parse_stmt();
		
		if(stmt == 0) {
			break;
		}
		
		if(count) {
			last->next = stmt;
			last = stmt;
		}
		else {
			first = stmt;
			last = stmt;
		}
		
		count ++;
	}
	
	Block *block = create(Block);
	block->first = first;
	block->last = last;
	block->count = count;
	block->scope = scope;
	scope = scope->parent;
	return block;
}

void parse_unit()
{
	filename = unit->filename;
	token = unit->tokens->first;
	line = token->line;
	pos = token->pos;
	unit->ast = parse_block();
	
	if(!is_kind(END)) {
		error("unexpected end of file");
	}
}
