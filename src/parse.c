char *assignops = "+= -= *= /= %= = ";
int inloop = 0;

Block *parse_block(Stmt *funchost);
Expr *parse_expr();
Stmt *parse_vardecl();

Token *next_token()
{
	Token *temp = token;
	token ++;
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

Token *parse_op(char *ops)
{
	if(!is_kind(PUNCT))
		return 0;
	
	while(*ops) {
		char *start = ops;
		
		while(*ops != ' ')
			ops ++;
		
		size_t op_len = ops - start;
		char op_buf[op_len + 1];
		memcpy(op_buf, start, op_len);
		op_buf[op_len] = 0;
		
		if(strcmp(token->text, op_buf) == 0) {
			return next_token();
		}
		
		ops ++;
	}
	
	return 0;
}

#include "expr.c"

Type *parse_primtype()
{
	PrimType primtype;
	
	if(is_keyword("i64") || is_keyword("int"))
		primtype = I64;
	else if(is_keyword("i32"))
		primtype = I32;
	else if(is_keyword("i16"))
		primtype = I16;
	else if(is_keyword("i8"))
		primtype = I8;
	else if(is_keyword("u64"))
		primtype = U64;
	else if(is_keyword("u32"))
		primtype = U32;
	else if(is_keyword("u16"))
		primtype = U16;
	else if(is_keyword("u8"))
		primtype = U8;
	else if(is_keyword("f64") || is_keyword("float"))
		primtype = F64;
	else if(is_keyword("f32"))
		primtype = F32;
	else if(is_kind(IDENT)) {
		Type *type = create(Type);
		type->kind = NAMEDTYPE;
		type->ident = next_token();
		return type;
	}
	else
		return 0;
	
	Type *type = create(Type);
	type->kind = PRIMTYPE;
	type->primtype = primtype;
	next_token();
	return type;
}

Type *parse_type()
{
	if(parse_punct(">")) {
		Type *type = parse_type();
		
		if(type == 0)
			error("expected pointer base type");
		
		Type *ptrtype = create(Type);
		ptrtype->kind = PTRTYPE;
		ptrtype->child = type;
		return ptrtype;
	}
	else if(parse_punct("[")) {
		Token *count = parse_kind(INTEGER);
		
		if(count == 0)
			error("expected integer after '['");
		
		if(parse_punct("]") == 0)
			error("expected ']' after integer");
		
		Type *child = parse_type();
		
		if(child == 0)
			error("expected base type after array dimension");
		
		Type *type = create(Type);
		type->kind = ARRAYTYPE;
		type->count = count->val;
		type->child = child;
		return type;
	}
	
	return parse_primtype();
}

Stmt *parse_import()
{
	if(parse_keyword("import") == 0)
		return 0;
	
	if(scope->parent)
		error("imports can only be used in global scope");
	
	Token *name = parse_kind(STRING);
	
	if(name == 0)
		error("expected string after 'import'");
	
	Stmt *import = create(Stmt);
	import->kind = IMPORT;
	import->string = name;
	return import;
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
	
	if(parse_punct("(") == 0)
		error("expected '(' after function name");
	
	Stmt *first = 0;
	Stmt *last = 0;
	size_t param_count = 0;
	
	while(1) {
		Stmt *param = parse_vardecl();
		
		if(param == 0)
			break;
		
		if(param->expr)
			error("parameter can not have an initializer");
		
		param->isparam = 1;
		
		if(first) {
			last->next = param;
			last = param;
		}
		else {
			first = param;
			last = param;
		}
		
		param_count ++;
		
		if(parse_punct(",") == 0)
			break;
	}
	
	if(parse_punct(")") == 0)
		error("expected ')' after parameter list");
	
	Type *type = 0;
	
	if(parse_punct(":")) {
		type = parse_type();
		
		if(type == 0)
			error("expected type after ':'");
	}
	
	if(parse_punct("{") == 0)
		error("expected '{' after function head");
	
	Stmt *funcdecl = create(Stmt);
	Block *body = parse_block(funcdecl);
	
	if(parse_punct("}") == 0)
		error("expected '}' after function statements");
	
	funcdecl->kind = FUNCDECL;
	funcdecl->ident = ident;
	funcdecl->type = type;
	funcdecl->body = body;
	funcdecl->param = first;
	funcdecl->param_count = param_count;
	return funcdecl;
}

Stmt *parse_assign()
{
	Token *start = token;
	Expr *target = parse_target();
	
	if(target == 0)
		return 0;
	
	Token *op = parse_op(assignops);
	
	if(op == 0) {
		seek_token(start);
		return 0;
	}
	
	if(target->islvalue == 0)
		error("target is not assignable");
	
	Expr *expr = parse_expr();
	
	if(expr == 0)
		error("expected expression after '%s'", op->text);
	
	Stmt *assign = create(Stmt);
	assign->kind = ASSIGN;
	assign->target = target;
	assign->expr = expr;
	assign->op = op;
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
	return vardecl;
}

Stmt *parse_callstmt()
{
	Expr *expr = parse_call();
	
	if(expr == 0)
		return 0;
	
	expr->iscallstmt = 1;
	Stmt *call = create(Stmt);
	call->kind = CALLSTMT;
	call->expr = expr;
	return call;
}

Stmt *parse_ifstmt()
{
	if(parse_keyword("if") == 0)
		return 0;
	
	Expr *expr = parse_expr();
	
	if(expr == 0)
		error("expected condition after 'if'");
	
	if(parse_punct("{") == 0)
		error("expected '{' after condition");
	
	Block *body = parse_block(0);
	
	if(parse_punct("}") == 0)
		error("expected '}' after if body");
	
	Stmt *ifstmt = create(Stmt);
	ifstmt->kind = IFSTMT;
	ifstmt->expr = expr;
	ifstmt->body = body;
	return ifstmt;
}

Stmt *parse_whilestmt()
{
	if(parse_keyword("while") == 0)
		return 0;
	
	Expr *expr = parse_expr();
	
	if(expr == 0)
		error("expected condition after 'while'");
	
	if(parse_punct("{") == 0)
		error("expected '{' after condition");
	
	int old_inloop = inloop;
	inloop = 1;
	Block *body = parse_block(0);
	inloop = old_inloop;
	
	if(parse_punct("}") == 0)
		error("expected '}' after while body");
	
	Stmt *whilestmt = create(Stmt);
	whilestmt->kind = WHILESTMT;
	whilestmt->expr = expr;
	whilestmt->body = body;
	return whilestmt;
}

Stmt *parse_print()
{
	if(parse_keyword("print") == 0)
		return 0;
	
	Expr *first = 0;
	Expr *last = 0;
	size_t count = 0;
	
	while(1) {
		Expr *item = parse_expr();
		
		if(item == 0)
			break;
		
		if(first) {
			last->next = item;
			last = item;
		}
		else {
			first = item;
			last = item;
		}
		
		count ++;
		
		if(parse_punct(",") == 0)
			break;
	}
	
	if(first == 0)
		error("expected at least one expression after 'print'");
	
	Stmt *print = create(Stmt);
	print->kind = PRINT;
	print->expr = first;
	print->param_count = count;
	return print;
}

Stmt *parse_return()
{
	if(parse_keyword("return") == 0)
		return 0;
	
	if(scope->funchost == 0)
		error("return statement can only be used inside a function");
	
	Stmt *stmt = create(Stmt);
	stmt->kind = RETURN;
	Expr *expr = parse_expr();
	
	if(expr == 0)
		return stmt;
	
	stmt->expr = expr;
	return stmt;
}

Stmt *parse_export()
{
	if(parse_keyword("export") == 0)
		return 0;
	
	if(scope->parent)
		error("exports can only be declared in global scope");
	
	Stmt *stmt;
	
	(stmt = parse_funcdecl()) ||
	(stmt = parse_vardecl()) ||
	0;
	
	if(stmt == 0)
		error("expected declaration after 'export'");
	
	stmt->exported = 1;
	stmt->exporthash = unit->hash;
	return stmt;
}

Stmt *parse_break()
{
	if(parse_keyword("break") == 0)
		return 0;
	
	if(!inloop)
		error("break can only be used inside a loop");
	
	Stmt *stmt = create(Stmt);
	stmt->kind = BREAK;
	return stmt;
}

Stmt *parse_continue()
{
	if(parse_keyword("continue") == 0)
		return 0;
	
	if(!inloop)
		error("continue can only be used inside a loop");
	
	Stmt *stmt = create(Stmt);
	stmt->kind = CONTINUE;
	return stmt;
}

Block *parse_struct_block()
{
	Stmt *first = 0;
	Stmt *last = 0;
	size_t count = 0;
	Scope *blockscope = create(Scope);
	blockscope->parent = scope;
	scope = blockscope;
	
	while(1) {
		Stmt *stmt = parse_vardecl();
		
		if(stmt == 0) {
			break;
		}
		
		if(stmt->expr)
			error("structure member initialization is not supported");
		
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

Stmt *parse_structdecl()
{
	if(parse_keyword("struct") == 0)
		return 0;
	
	Token *ident = parse_kind(IDENT);
	
	if(ident == 0)
		error("expected identifier after 'struct'");
	
	if(parse_punct("{") == 0)
		error("expected '{' after structure name");
	
	Block *body = parse_struct_block();
	
	if(parse_punct("}") == 0)
		error("expected '}' after structure definition");
	
	Stmt *stmt = create(Stmt);
	stmt->kind = STRUCTDECL;
	stmt->ident = ident;
	stmt->body = body;
	return stmt;
}

Stmt *parse_stmt()
{
	Stmt *stmt;
	
	(stmt = parse_export()) ||
	(stmt = parse_import()) ||
	(stmt = parse_ifstmt()) ||
	(stmt = parse_whilestmt()) ||
	(stmt = parse_funcdecl()) ||
	(stmt = parse_structdecl()) ||
	(stmt = parse_print()) ||
	(stmt = parse_return()) ||
	(stmt = parse_assign()) ||
	(stmt = parse_vardecl()) ||
	(stmt = parse_callstmt()) ||
	(stmt = parse_break()) ||
	(stmt = parse_continue()) ||
	0;
	
	return stmt;
}

Block *parse_block(Stmt *funchost)
{
	Stmt *first = 0;
	Stmt *last = 0;
	size_t count = 0;
	Scope *blockscope = create(Scope);
	blockscope->parent = scope;
	scope = blockscope;
	
	scope->funchost = funchost
		? funchost
		: scope->parent
			? scope->parent->funchost
			: 0;
	
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
	token = unit->tokens;
	line = token->line;
	pos = token->pos;
	scope = 0;
	inloop = 0;
	unit->ast = parse_block(0);
	
	if(!is_kind(END)) {
		error("unexpected end of file");
	}
}