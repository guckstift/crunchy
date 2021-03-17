void analyze_block(Block *block);

Symbol *lookup(Token *ident)
{
	for(Symbol *symbol = scope->first; symbol; symbol = symbol->next) {
		if(strcmp(symbol->ident->text, ident->text) == 0) {
			return symbol;
		}
	}
	
	for(Symbol *symbol = scope->first_import; symbol; symbol = symbol->next) {
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

void declare_import(Stmt *decl)
{
	Symbol *symbol = create(Symbol);
	symbol->decl = decl;
	symbol->ident = decl->ident;
	
	if(scope->import_count) {
		scope->last_import->next = symbol;
		scope->last_import = symbol;
	}
	else {
		scope->first_import = symbol;
		scope->last_import = symbol;
	}
	
	scope->import_count ++;
}

void scan_block_decls(Block *block)
{
	Scope *oldscope = scope;
	scope = block->scope;
	
	for(Stmt *stmt = block->first; stmt; stmt = stmt->next) {
		if(stmt->kind == VARDECL) {
			Token *ident = stmt->ident;
			
			if(lookup(ident))
				error_at(ident, "'%s' is already declared", ident->text);
			
			declare(stmt);
		}
		else if(stmt->kind == FUNCDECL) {
			Token *ident = stmt->ident;
			
			if(lookup(ident))
				error_at(ident, "'%s' is already declared", ident->text);
			
			declare(stmt);
			scan_block_decls(stmt->body);
		}
		else if(stmt->kind == IMPORT)
			stmt->unit = do_import(stmt->string->text);
		else if(stmt->kind == IFSTMT)
			scan_block_decls(stmt->body);
		else if(stmt->kind == WHILESTMT)
			scan_block_decls(stmt->body);
	}
	
	scope = oldscope;
}

void scan_decls()
{
	scan_block_decls(unit->ast);
}

int types_equal(Type *t1, Type *t2)
{
	if(t1->kind == PRIMTYPE && t2->kind == PRIMTYPE)
		return t1->primtype == t2->primtype;
	
	if(t1->kind == PTRTYPE && t2->kind == PTRTYPE)
		return types_equal(t1->child, t2->child);
	
	return 0;
}

Type *analyze_var_ident(Token *ident)
{
	seek_token(ident);
	Symbol *symbol = lookup_rec(ident);
	
	if(symbol == 0)
		error("'%s' is not defined", ident->text);
	
	Stmt *decl = symbol->decl;
	
	if(decl->kind != VARDECL)
		error("'%s' is not a variable", ident->text);
	
	if(decl->state != RESOLVED)
		error("'%s' is not initialized yet", ident->text);
	
	return decl->type;
}

void analyze_expr(Expr *expr)
{
	if(expr->kind == PRIM) {
		Token *prim = expr->prim;
		
		if(prim->kind == INTEGER) {
			expr->type = create(Type);
			expr->type->kind = PRIMTYPE;
			expr->type->primtype = I64;
		}
		else if(prim->kind == IDENT) {
			Type *type = analyze_var_ident(prim);
			expr->type = type;
		}
	}
	else if(expr->kind == PTR) {
		analyze_expr(expr->child);
		Type *type = create(Type);
		type->kind = PTRTYPE;
		type->child = expr->child->type;
		expr->type = type;
	}
	else if(expr->kind == DEREF) {
		Expr *child = expr->child;
		analyze_expr(child);
		Type *child_type = child->type;
		
		if(child_type->kind != PTRTYPE)
			error("can only dereference pointers");
		
		expr->type = child_type->child;
	}
	else if(expr->kind == CHAIN) {
		analyze_expr(expr->left);
		analyze_expr(expr->right);
		
		if(expr->left->type->kind == PTRTYPE)
			error("left side of '%s' is a pointer", expr->op->text);
		
		if(expr->right->type->kind == PTRTYPE)
			error("right side of '%s' is a pointer", expr->op->text);
		
		expr->type = expr->left->type;
	}
}

void analyze_stmt(Stmt *stmt)
{
	if(stmt->state != UNRESOLVED)
		return;
	
	stmt->state = RESOLVING;
	
	if(stmt->kind == ASSIGN) {
		Token *ident = stmt->ident;
		Type *ident_type = analyze_var_ident(ident);
		analyze_expr(stmt->expr);
		
		if(types_equal(ident_type, stmt->expr->type) == 0)
			error("types are not equal");
	}
	else if(stmt->kind == VARDECL) {
		if(stmt->expr) {
			analyze_expr(stmt->expr);
			
			if(stmt->type == 0)
				stmt->type = stmt->expr->type;
			
			if(types_equal(stmt->type, stmt->expr->type) == 0)
				error("types are not equal");
		}
	}
	else if(stmt->kind == FUNCDECL)
		analyze_block(stmt->body);
	else if(stmt->kind == IFSTMT)
		analyze_block(stmt->body);
	else if(stmt->kind == WHILESTMT)
		analyze_block(stmt->body);
	else if(stmt->kind == CALL) {
		Token *ident = stmt->ident;
		Symbol *symbol = lookup_rec(ident);
		seek_token(ident);
		
		if(symbol == 0)
			error("'%s' is not defined", ident->text);
		
		Stmt *decl = symbol->decl;
		
		if(decl->kind != FUNCDECL)
			error("'%s' is not a function", ident->text);
		
		analyze_stmt(decl);
	}
	else if(stmt->kind == PRINT) {
		analyze_expr(stmt->expr);
		
		if(stmt->expr->type->kind != PRIMTYPE)
			error("can only print primitive types");
	}
	else if(stmt->kind == RETURN) {
		Expr *expr = stmt->expr;
		
		if(expr) {
			analyze_expr(expr);
			Stmt *func = scope->funchost;
			
			if(func->type == 0)
				error("function should not return a value");
			
			if(!types_equal(expr->type, func->type))
				error("returning the wrong type");
		}
	}
	
	stmt->state = RESOLVED;
}

void analyze_block(Block *block)
{
	Scope *oldscope = scope;
	scope = block->scope;
	
	for(Stmt *stmt = block->first; stmt; stmt = stmt->next)
		analyze_stmt(stmt);
	
	scope = oldscope;
}

void analyze_unit()
{
	filename = unit->filename;
	line = 1;
	pos = 1;
	scope = 0;
	scan_decls();
	line = 1;
	pos = 1;
	scope = 0;
	analyze_block(unit->ast);
}
