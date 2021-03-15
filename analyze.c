void analyze_block(Block *block);

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
	else if(expr->kind == CHAIN) {
		analyze_expr(expr->left);
		analyze_expr(expr->right);
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
		analyze_var_ident(ident);
		analyze_expr(stmt->expr);
	}
	else if(stmt->kind == VARDECL) {
		if(stmt->expr) {
			analyze_expr(stmt->expr);
			
			if(stmt->type == 0)
				stmt->type = stmt->expr->type;
		}
	}
	else if(stmt->kind == FUNCDECL)
		analyze_block(stmt->body);
	else if(stmt->kind == CALL) {
		Token *ident = stmt->ident;
		Symbol *symbol = lookup_rec(ident);
		
		if(symbol == 0)
			error("'%s' is not defined", ident->text);
		
		Stmt *decl = symbol->decl;
		
		if(decl->kind != FUNCDECL)
			error("'%s' is not a function", ident->text);
		
		analyze_stmt(decl);
	}
	else if(stmt->kind == PRINT)
		analyze_expr(stmt->expr);
	
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
	analyze_block(unit->ast);
}
