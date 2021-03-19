
void scan_block_decls(Block *block);
void analyze_block(Block *block);
void analyze_stmt(Stmt *stmt);
Expr *adjust_assign_value(Expr *expr, Type *target_type);

Symbol *lookup(Token *ident)
{
	assert(ident);
	
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
	assert(ident);
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
	assert(decl);
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
	assert(decl);
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

void scan_stmt_decls(Stmt *stmt)
{
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
		Scope *oldscope = scope;
		scope = stmt->body->scope;
		
		for(Stmt *param = stmt->param; param; param = param->next) {
			if(param->type->kind == ARRAYTYPE)
				error("function parameters can not be arrays");
			
			declare(param);
		}
		
		scope = oldscope;
		scan_block_decls(stmt->body);
	}
	else if(stmt->kind == IMPORT)
		stmt->unit = do_import(stmt->string->text);
	else if(stmt->kind == IFSTMT)
		scan_block_decls(stmt->body);
	else if(stmt->kind == WHILESTMT)
		scan_block_decls(stmt->body);
}

void scan_block_decls(Block *block)
{
	assert(block);
	Scope *oldscope = scope;
	scope = block->scope;
	
	for(Stmt *stmt = block->first; stmt; stmt = stmt->next) {
		scan_stmt_decls(stmt);
	}
	
	scope = oldscope;
}

void scan_decls()
{
	scan_block_decls(unit->ast);
}

int types_equal(Type *t1, Type *t2)
{
	assert(t1);
	assert(t2);
	
	if(t1->kind == PRIMTYPE && t2->kind == PRIMTYPE)
		return t1->primtype == t2->primtype;
	
	if(t1->kind == PTRTYPE && t2->kind == PTRTYPE)
		return types_equal(t1->child, t2->child);
	
	if(t1->kind == ARRAYTYPE && t2->kind == ARRAYTYPE)
		return t1->count == t2->count && types_equal(t1->child, t2->child);
	
	return 0;
}

Type *analyze_var_ident(Token *ident)
{
	assert(ident);
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

Expr *unwrap_ptr_to_array(Expr *expr)
{
	assert(expr);
	assert(expr->type);
	Type *type = expr->type;
	size_t deref_count = 0;
	
	while(type->kind == PTRTYPE) {
		type = type->child;
		deref_count ++;
	}
	
	if(type->kind != ARRAYTYPE)
		return expr;
	
	while(deref_count > 0) {
		Expr *new_expr = create(Expr);
		new_expr->kind = DEREF;
		new_expr->child = expr;
		new_expr->type = expr->type->child;
		expr = new_expr;
		deref_count --;
	}
	
	return expr;
}

void analyze_expr(Expr *expr)
{
	assert(expr);
	
	if(expr->kind == PRIM) {
		Token *prim = expr->prim;
		
		if(prim->kind == INTEGER) {
			expr->type = create(Type);
			expr->type->kind = PRIMTYPE;
			expr->type->primtype = I64;
			expr->iconst = prim->val;
		}
		else if(prim->kind == FLOAT) {
			expr->type = create(Type);
			expr->type->kind = PRIMTYPE;
			expr->type->primtype = F64;
		}
		else if(prim->kind == IDENT) {
			Type *type = analyze_var_ident(prim);
			expr->type = type;
		}
	}
	else if(expr->kind == UNARY) {
		analyze_expr(expr->child);
		expr->type = expr->child->type;
		
		if(strcmp(expr->op->text, "-") == 0)
			expr->iconst = - expr->child->iconst;
	}
	else if(expr->kind == ARRAY) {
		Type *itemtype = 0;
		
		for(Expr *item = expr->child; item; item = item->next) {
			analyze_expr(item);
			
			if(itemtype) {
				if(!types_equal(itemtype, item->type))
					error("array item types are not consistent");
			}
			else
				itemtype = item->type;
		}
		
		Type *type = create(Type);
		type->kind = ARRAYTYPE;
		type->count = expr->length;
		type->child = itemtype;
		expr->type = type;
	}
	else if(expr->kind == CALL) {
		Token *ident = expr->ident;
		Symbol *symbol = lookup_rec(ident);
		seek_token(ident);
		
		if(symbol == 0)
			error("'%s' is not defined", ident->text);
		
		Stmt *decl = symbol->decl;
		
		if(decl->kind != FUNCDECL)
			error("'%s' is not a function", ident->text);
		
		expr->type = decl->type;
		analyze_stmt(decl);
		
		if(!expr->iscallstmt && expr->type == 0)
			error("function has no return value");
		
		if(expr->length != decl->param_count)
			error("argument count does not fit");
		
		Stmt *param = decl->param;
		Expr *last = 0;
		
		for(Expr *arg = expr->child; arg; arg = arg->next) {
			analyze_expr(arg);
			Type *param_type = param->type;
			Expr *new_arg = adjust_assign_value(arg, param_type);
			Type *arg_type = new_arg->type;
			
			if(!types_equal(arg_type, param_type))
				error("argument type does not fit");
			
			if(new_arg != arg) {
				if(last)
					last->next = new_arg;
				else
					expr->child = new_arg;
				
				new_arg->next = arg->next;
				arg = new_arg;
			}
			
			last = arg;
			param = param->next;
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
	else if(expr->kind == ADDRESS) {
		Expr *child = expr->child;
		analyze_expr(child);
		Type *child_type = child->type;
		
		if(child_type->kind != PTRTYPE)
			error("can only take the address of pointers");
		
		expr->type = create(Type);
		expr->type->kind = PRIMTYPE;
		expr->type->primtype = U64;
	}
	else if(expr->kind == CHAIN) {
		analyze_expr(expr->left);
		analyze_expr(expr->right);
		
		if(expr->left->type->kind == PTRTYPE)
			error("left side of '%s' is a pointer", expr->op->text);
		
		if(expr->right->type->kind == PTRTYPE)
			error("right side of '%s' is a pointer", expr->op->text);
		
		expr->type = expr->left->type;
		
		if(strcmp(expr->op->text, "+") == 0)
			expr->iconst = expr->left->iconst + expr->right->iconst;
		else if(strcmp(expr->op->text, "-") == 0)
			expr->iconst = expr->left->iconst - expr->right->iconst;
		else if(strcmp(expr->op->text, "*") == 0)
			expr->iconst = expr->left->iconst * expr->right->iconst;
		else if(strcmp(expr->op->text, "/") == 0)
			expr->iconst = expr->left->iconst / expr->right->iconst;
		else if(strcmp(expr->op->text, "%") == 0)
			expr->iconst = expr->left->iconst % expr->right->iconst;
		else if(strcmp(expr->op->text, "&&") == 0)
			expr->iconst = expr->left->iconst && expr->right->iconst;
		else if(strcmp(expr->op->text, "||") == 0)
			expr->iconst = expr->left->iconst || expr->right->iconst;
		else if(strcmp(expr->op->text, "==") == 0)
			expr->iconst = expr->left->iconst == expr->right->iconst;
		else if(strcmp(expr->op->text, "!=") == 0)
			expr->iconst = expr->left->iconst != expr->right->iconst;
		else if(strcmp(expr->op->text, "<=") == 0)
			expr->iconst = expr->left->iconst <= expr->right->iconst;
		else if(strcmp(expr->op->text, ">=") == 0)
			expr->iconst = expr->left->iconst >= expr->right->iconst;
		else if(strcmp(expr->op->text, "<") == 0)
			expr->iconst = expr->left->iconst < expr->right->iconst;
		else if(strcmp(expr->op->text, ">") == 0)
			expr->iconst = expr->left->iconst > expr->right->iconst;
	}
	else if(expr->kind == SUBSCRIPT) {
		analyze_expr(expr->left);
		analyze_expr(expr->right);
		expr->left = unwrap_ptr_to_array(expr->left);
		
		if(expr->left->type->kind != ARRAYTYPE)
			error("left side of subscript is not an array");
		
		if(expr->right->type->kind != PRIMTYPE)
			error("index type is not primitive");
		
		if(expr->right->isconst) {
			if(expr->right->iconst >= expr->left->type->count)
				error("array bounds exceeded");
		}
		
		expr->type = expr->left->type->child;
	}
}

Expr *unwrap_pointer(Expr *expr)
{
	while(expr->type->kind == PTRTYPE) {
		Expr *new_expr = create(Expr);
		new_expr->kind = DEREF;
		new_expr->child = expr;
		new_expr->type = expr->type->child;
		expr = new_expr;
	}
	
	return expr;
}

Expr *adjust_assign_target(Expr *target, Type *expr_type)
{
	assert(target);
	assert(target->type);
	assert(expr_type);
	Type *type = target->type;
	size_t deref_count = 0;
	
	while(type->kind == PTRTYPE && !types_equal(type, expr_type)) {
		type = type->child;
		deref_count ++;
	}
	
	if(!types_equal(type, expr_type))
		return target;
	
	while(deref_count > 0) {
		Expr *new_target = create(Expr);
		new_target->kind = DEREF;
		new_target->child = target;
		new_target->type = target->type->child;
		target = new_target;
		deref_count --;
	}
	
	return target;
}

Expr *adjust_assign_value(Expr *expr, Type *target_type)
{
	assert(expr);
	assert(target_type);
	Type *type = expr->type;
	size_t deref_count = 0;
	
	if(target_type->kind == PTRTYPE && !types_equal(target_type, expr->type)) {
		if(types_equal(target_type->child, expr->type) && expr->islvalue) {
			Type *new_expr_type = create(Type);
			new_expr_type->kind = PTRTYPE;
			new_expr_type->child = expr->type;
			Expr *new_expr = create(Expr);
			new_expr->kind = PTR;
			new_expr->child = expr;
			new_expr->type = new_expr_type;
			return new_expr;
		}
	}
	
	while(type->kind == PTRTYPE && !types_equal(target_type, type)) {
		type = type->child;
		deref_count ++;
	}
	
	if(!types_equal(target_type, type))
		return expr;
	
	while(deref_count > 0) {
		Expr *new_expr = create(Expr);
		new_expr->kind = DEREF;
		new_expr->child = expr;
		new_expr->type = expr->type->child;
		expr = new_expr;
		deref_count --;
	}
	
	return expr;
}

void analyze_stmt(Stmt *stmt)
{
	assert(stmt);
	
	if(stmt->state != UNRESOLVED)
		return;
	
	stmt->state = RESOLVING;
	
	if(stmt->kind == ASSIGN) {
		analyze_expr(stmt->target);
		analyze_expr(stmt->expr);
		
		if(strcmp(stmt->op->text, "=") != 0)
			stmt->target = unwrap_pointer(stmt->target);
		else
			stmt->target = adjust_assign_target(stmt->target, stmt->expr->type);
		
		stmt->expr = adjust_assign_value(stmt->expr, stmt->target->type);
		
		if(stmt->target->type->kind == ARRAYTYPE)
			error("reassignment of arrays is not supported");
		
		if(types_equal(stmt->target->type, stmt->expr->type) == 0)
			error("types are not equal");
	}
	else if(stmt->kind == VARDECL) {
		if(stmt->expr) {
			analyze_expr(stmt->expr);
			
			if(stmt->type == 0)
				stmt->type = stmt->expr->type;
			
			stmt->expr = adjust_assign_value(stmt->expr, stmt->type);
			
			if(types_equal(stmt->type, stmt->expr->type) == 0)
				error("types are not equal");
			
			if(stmt->expr)
				if(stmt->type->kind == ARRAYTYPE && stmt->expr->kind != ARRAY)
					error("can only initialize array with a literal");
		}
	}
	else if(stmt->kind == FUNCDECL) {
		if(stmt->type && stmt->type->kind == ARRAYTYPE)
			error("functions can not return arrays");
		
		Scope *oldscope = scope;
		scope = stmt->body->scope;
		
		for(Stmt *param = stmt->param; param; param = param->next)
			analyze_stmt(param);
		
		scope = oldscope;
		analyze_block(stmt->body);
	}
	else if(stmt->kind == IFSTMT)
		analyze_block(stmt->body);
	else if(stmt->kind == WHILESTMT)
		analyze_block(stmt->body);
	else if(stmt->kind == CALLSTMT)
		analyze_expr(stmt->expr);
	else if(stmt->kind == PRINT) {
		Expr *last = 0;
		
		for(Expr *item = stmt->expr; item; item = item->next) {
			analyze_expr(item);
			Expr *unwrapped = unwrap_pointer(item);
			
			if(unwrapped != item) {
				if(last)
					last->next = unwrapped;
				else
					stmt->expr = unwrapped;
				
				unwrapped->next = item->next;
				item = unwrapped;
			}
			
			if(item->type->kind != PRIMTYPE)
				error("can only print primitive types");
			
			last = item;
		}
	}
	else if(stmt->kind == RETURN) {
		Expr *expr = stmt->expr;
		Stmt *func = scope->funchost;
		
		if(expr) {
			analyze_expr(expr);
			
			if(func->type == 0)
				error("function should not return a value");
			
			stmt->expr = adjust_assign_value(stmt->expr, func->type);
			
			if(!types_equal(stmt->expr->type, func->type))
				error("returning the wrong type");
		}
		else {
			if(func->type)
				error("function should return with a value");
		}
	}
	
	stmt->state = RESOLVED;
}

void analyze_block(Block *block)
{
	assert(block);
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
