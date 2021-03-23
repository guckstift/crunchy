
void scan_block_decls(Block *block);
void analyze_block(Block *block);
void analyze_stmt(Stmt *stmt);
Expr *adjust_assign_value(Expr *expr, Type *target_type);

Symbol *lookup(char *name)
{
	assert(name);
	
	for(Symbol *symbol = scope->first; symbol; symbol = symbol->next) {
		if(symbol->name == name) {
			return symbol;
		}
	}
	
	for(Symbol *symbol = scope->first_import; symbol; symbol = symbol->next) {
		if(symbol->name == name) {
			return symbol;
		}
	}
	
	return 0;
}

Symbol *lookup_rec(char *name)
{
	assert(name);
	Symbol *symbol = lookup(name);
	
	if(symbol) {
		return symbol;
	}
	
	if(scope->parent) {
		Scope *backup = scope;
		scope = scope->parent;
		symbol = lookup_rec(name);
		scope = backup;
	}
	
	return symbol;
}

void declare(Stmt *decl)
{
	assert(decl);
	Symbol *symbol = create(Symbol);
	symbol->decl = decl;
	symbol->name = decl->name;
	
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
	symbol->name = decl->name;
	
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

Expr *create_expr_wrap(Kind kind, Expr *child)
{
	Expr *wrap = create_expr(kind);
	wrap->child = child;
	wrap->line = child->line;
	wrap->pos = child->pos;
	return wrap;
}

void scan_stmt_decls(Stmt *stmt)
{
	if(
		stmt->kind == VARDECL || stmt->kind == FUNCDECL ||
		stmt->kind == STRUCTDECL
	) {
		char *name = stmt->name;
		
		if(lookup(name))
			error_stmt(stmt, "'%s' is already declared", name);
		
		declare(stmt);
	}
	
	if(stmt->kind == FUNCDECL) {
		Scope *oldscope = scope;
		scope = stmt->body->scope;
		
		for(Stmt *param = stmt->param; param; param = param->next) {
			if(param->type->kind == ARRAYTYPE)
				error_stmt(param, "function parameters can not be arrays");
			
			declare(param);
		}
		
		scope = oldscope;
		scan_block_decls(stmt->body);
	}
	else if(stmt->kind == STRUCTDECL)
		scan_block_decls(stmt->body);
	else if(stmt->kind == IMPORT)
		stmt->unit = do_import(stmt->name);
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
	
	if(t1->kind == STRUCTTYPE && t2->kind == STRUCTTYPE)
		return t1->typedecl == t2->typedecl;
	
	if(t1->kind == PTRTYPE && t2->kind == PTRTYPE)
		return types_equal(t1->child, t2->child);
	
	if(t1->kind == SLICETYPE && t2->kind == SLICETYPE)
		return types_equal(t1->child, t2->child);
	
	if(t1->kind == ARRAYTYPE && t2->kind == ARRAYTYPE)
		return t1->count == t2->count && types_equal(t1->child, t2->child);
	
	return 0;
}

Type *analyze_var_name(char *name)
{
	assert(name);
	Symbol *symbol = lookup_rec(name);
	
	if(symbol == 0)
		error("'%s' is not defined", name);
	
	Stmt *decl = symbol->decl;
	
	if(decl->kind != VARDECL)
		error("'%s' is not a variable", name);
	
	if(decl->state != RESOLVED)
		error("'%s' is not initialized yet", name);
	
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
		Expr *new_expr = create_expr_wrap(DEREF, expr);
		new_expr->type = expr->type->child;
		expr = new_expr;
		deref_count --;
	}
	
	return expr;
}

Expr *unwrap_ptr_to_struct(Expr *expr)
{
	assert(expr);
	assert(expr->type);
	Type *type = expr->type;
	size_t deref_count = 0;
	
	while(type->kind == PTRTYPE) {
		type = type->child;
		deref_count ++;
	}
	
	if(type->kind != STRUCTTYPE)
		return expr;
	
	while(deref_count > 0) {
		Expr *new_expr = create_expr_wrap(DEREF, expr);
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
		Kind prim = expr->prim;
		
		if(prim == INTEGER) {
			expr->type = create_type(PRIMTYPE);
			expr->type->primtype = I64;
			expr->iconst = expr->val;
		}
		else if(prim == FLOAT) {
			expr->type = create_type(PRIMTYPE);
			expr->type->primtype = F64;
		}
		else if(prim == IDENT) {
			Type *type = analyze_var_name(expr->name);
			expr->type = type;
		}
	}
	else if(expr->kind == UNARY) {
		analyze_expr(expr->child);
		expr->type = expr->child->type;
		
		if(expr->op == PN_MINUS)
			expr->iconst = - expr->child->iconst;
	}
	else if(expr->kind == ARRAY) {
		Type *itemtype = 0;
		
		for(Expr *item = expr->child; item; item = item->next) {
			analyze_expr(item);
			
			if(itemtype) {
				if(!types_equal(itemtype, item->type))
					error_expr(item, "array item types are not consistent");
			}
			else
				itemtype = item->type;
		}
		
		Type *type = create_type(ARRAYTYPE);
		type->count = expr->length;
		type->child = itemtype;
		expr->type = type;
	}
	else if(expr->kind == CALL) {
		char *name = expr->name;
		Symbol *symbol = lookup_rec(name);
		
		if(symbol == 0)
			error_expr(expr, "'%s' is not defined", name);
		
		Stmt *decl = symbol->decl;
		
		if(decl->kind != FUNCDECL)
			error_expr(expr, "'%s' is not a function", name);
		
		expr->type = decl->type;
		analyze_stmt(decl);
		
		if(!expr->iscallstmt && expr->type == 0)
			error_expr(expr, "function has no return value");
		
		if(expr->length != decl->param_count)
			error_expr(expr, "argument count does not fit");
		
		Stmt *param = decl->param;
		Expr *last = 0;
		
		for(Expr *arg = expr->child; arg; arg = arg->next) {
			analyze_expr(arg);
			Type *param_type = param->type;
			Expr *new_arg = adjust_assign_value(arg, param_type);
			Type *arg_type = new_arg->type;
			
			if(!types_equal(arg_type, param_type))
				error_expr(arg, "argument type does not fit");
			
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
		Type *type = create_type(PTRTYPE);
		type->child = expr->child->type;
		expr->type = type;
	}
	else if(expr->kind == DEREF) {
		Expr *child = expr->child;
		analyze_expr(child);
		Type *child_type = child->type;
		
		if(child_type->kind != PTRTYPE)
			error_expr(child, "can only dereference pointers");
		
		expr->type = child_type->child;
	}
	else if(expr->kind == ADDRESS) {
		Expr *child = expr->child;
		analyze_expr(child);
		Type *child_type = child->type;
		
		if(child_type->kind != PTRTYPE)
			error_expr(child, "can only take the address of pointers");
		
		expr->type = create_type(PRIMTYPE);
		expr->type->primtype = U64;
	}
	else if(expr->kind == CHAIN) {
		analyze_expr(expr->left);
		analyze_expr(expr->right);
		
		if(expr->left->type->kind == PTRTYPE)
			error_expr(
				expr->left,
				"left side of '%s' is a pointer", puncts[expr->op]
			);
		
		if(expr->right->type->kind == PTRTYPE)
			error_expr(
				expr->right,
				"right side of '%s' is a pointer", puncts[expr->op]
			);
		
		expr->type = expr->left->type;
		
		switch(expr->op) {
		case PN_PLUS:
			expr->iconst = expr->left->iconst + expr->right->iconst;
			break;
		case PN_MINUS:
			expr->iconst = expr->left->iconst - expr->right->iconst;
			break;
		case PN_MUL:
			expr->iconst = expr->left->iconst * expr->right->iconst;
			break;
		case PN_DIV:
			expr->iconst = expr->left->iconst / expr->right->iconst;
			break;
		case PN_MOD:
			expr->iconst = expr->left->iconst % expr->right->iconst;
			break;
		case PN_AND:
			expr->iconst = expr->left->iconst && expr->right->iconst;
			break;
		case PN_OR:
			expr->iconst = expr->left->iconst || expr->right->iconst;
			break;
		case PN_EQU:
			expr->iconst = expr->left->iconst == expr->right->iconst;
			break;
		case PN_NEQU:
			expr->iconst = expr->left->iconst != expr->right->iconst;
			break;
		case PN_LTEQU:
			expr->iconst = expr->left->iconst <= expr->right->iconst;
			break;
		case PN_GTEQU:
			expr->iconst = expr->left->iconst >= expr->right->iconst;
			break;
		case PN_LT:
			expr->iconst = expr->left->iconst < expr->right->iconst;
			break;
		case PN_GT:
			expr->iconst = expr->left->iconst > expr->right->iconst;
			break;
		}
	}
	else if(expr->kind == SUBSCRIPT) {
		analyze_expr(expr->left);
		analyze_expr(expr->right);
		expr->left = unwrap_ptr_to_array(expr->left);
		Expr *left = expr->left;
		Expr *index = expr->right;
		Expr *slend = expr->slice_end;
		
		if(left->type->kind == ARRAYTYPE || left->type->kind == SLICETYPE) {
			if(index->type->kind != PRIMTYPE)
				error_expr(expr->right, "index type is not primitive");
			
			if(left->type->kind == ARRAYTYPE && expr->right->isconst) {
				if(expr->right->iconst >= expr->left->type->count)
					error_expr(expr->right, "array bounds exceeded");
			}
			
			expr->type = expr->left->type->child;
		}
		else
			error_expr(
				expr->left, "left side of subscript is not an array or slice"
			);
	}
	else if(expr->kind == SLICE) {
		analyze_expr(expr->left);
		analyze_expr(expr->right);
		analyze_expr(expr->slice_end);
		expr->left = unwrap_ptr_to_array(expr->left);
		
		if(
			expr->left->type->kind != ARRAYTYPE &&
			expr->left->type->kind != SLICETYPE
		)
			error_expr(expr->left, "left side of slice is not an array");
		
		if(expr->right->type->kind != PRIMTYPE)
			error_expr(expr->right, "slice start type is not primitive");
		
		if(expr->slice_end->type->kind != PRIMTYPE)
			error_expr(expr->slice_end, "slice end type is not primitive");
		
		if(expr->left->type->kind == ARRAYTYPE) {
			if(expr->right->isconst) {
				if(
					expr->right->iconst < 0 ||
					expr->right->iconst >= expr->left->type->count
				)
					error_expr(expr->right, "slice start out of bounds");
			}
			
			if(expr->slice_end->isconst) {
				if(expr->slice_end->iconst > expr->left->type->count)
					error_expr(expr->slice_end, "slice end out of bounds");
			}
			
			expr->type = create_type(SLICETYPE);
			expr->type->child = expr->left->type->child;
		}
		else {
			expr->type = expr->left->type;
		}
	}
	else if(expr->kind == MEMBER) {
		analyze_expr(expr->left);
		expr->left = unwrap_ptr_to_struct(expr->left);
		Type *left_type = expr->left->type;
		
		if(left_type->kind == SLICETYPE) {
			if(expr->right->name == kw_length) {
				expr->type = create_type(PRIMTYPE);
				expr->type->primtype = U64;
			}
			else
				error_expr(expr->left, "slice has no such member");
		}
		else if(left_type->kind == STRUCTTYPE) {
			Stmt *structdecl = left_type->typedecl;
			Scope *oldscope = scope;
			scope = structdecl->body->scope;
			analyze_expr(expr->right);
			expr->type = expr->right->type;
			scope = oldscope;
		}
		else
			error_expr(expr->left, "left side of '.' is not a structure");
	}
}

Expr *unwrap_pointer(Expr *expr)
{
	while(expr->type->kind == PTRTYPE) {
		Expr *new_expr = create_expr_wrap(DEREF, expr);
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
		Expr *new_target = create_expr_wrap(DEREF, target);
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
			Type *new_expr_type = create_type(PTRTYPE);
			new_expr_type->child = expr->type;
			Expr *new_expr = create_expr_wrap(PTR, expr);
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
		Expr *new_expr = create_expr_wrap(DEREF, expr);
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
		
		if(stmt->op == PN_ASSIGN)
			stmt->target = adjust_assign_target(stmt->target, stmt->expr->type);
		else
			stmt->target = unwrap_pointer(stmt->target);
		
		stmt->expr = adjust_assign_value(stmt->expr, stmt->target->type);
		
		if(stmt->target->type->kind == ARRAYTYPE)
			error_stmt(stmt, "reassignment of arrays is not supported");
		
		if(types_equal(stmt->target->type, stmt->expr->type) == 0)
			error_stmt(stmt, "types are not equal");
	}
	else if(stmt->kind == VARDECL) {
		if(stmt->type) {
			if(stmt->type->kind == NAMEDTYPE) {
				char *name = stmt->type->name;
				Symbol *sym = lookup_rec(name);
				
				if(sym == 0)
					error_stmt(
						stmt,
						"'%s' is an unknown type name", name
					);
				
				Stmt *decl = sym->decl;
				
				if(decl->kind != STRUCTDECL)
					error_stmt(
						stmt,
						"'%s' is not a struct type name", name
					);
				
				stmt->type->kind = STRUCTTYPE;
				stmt->type->typedecl = decl;
			}
		}
		
		if(stmt->expr) {
			analyze_expr(stmt->expr);
			
			if(stmt->type == 0)
				stmt->type = stmt->expr->type;
			
			stmt->expr = adjust_assign_value(stmt->expr, stmt->type);
			
			if(types_equal(stmt->type, stmt->expr->type) == 0)
				error_stmt(stmt, "types are not equal");
			
			if(stmt->expr)
				if(stmt->type->kind == ARRAYTYPE && stmt->expr->kind != ARRAY)
					error_stmt(
						stmt, "can only initialize array with a literal"
					);
		}
	}
	else if(stmt->kind == FUNCDECL) {
		if(stmt->type && stmt->type->kind == ARRAYTYPE)
			error_stmt(stmt, "functions can not return arrays");
		
		Scope *oldscope = scope;
		scope = stmt->body->scope;
		
		for(Stmt *param = stmt->param; param; param = param->next)
			analyze_stmt(param);
		
		scope = oldscope;
		analyze_block(stmt->body);
	}
	else if(stmt->kind == STRUCTDECL)
		analyze_block(stmt->body);
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
				error_expr(item, "can only print primitive types");
			
			last = item;
		}
	}
	else if(stmt->kind == RETURN) {
		Expr *expr = stmt->expr;
		Stmt *func = scope->funchost;
		
		if(expr) {
			analyze_expr(expr);
			
			if(func->type == 0)
				error_stmt(stmt, "function should not return a value");
			
			stmt->expr = adjust_assign_value(stmt->expr, func->type);
			
			if(!types_equal(stmt->expr->type, func->type))
				error_expr(expr, "returning the wrong type");
		}
		else {
			if(func->type)
				error_stmt(stmt, "function should return with a value");
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
