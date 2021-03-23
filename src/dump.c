char *primtype_names[] = {
	"u8", "u16", "u32", "u64",
	"i8", "i16", "i32", "i64",
	"f32", "f64",
};

void dump_block(Block *block);
void dump_expr(Expr *expr);

void dump_indent()
{
	for(int i=0; i<level; i++)
		printf("  ");
}

void dump_token(Token *token)
{
	if(token->kind == INTEGER)
		printf("%lu", token->val);
	else if(token->kind == FLOAT)
		printf("%s", d2s(token->fval, 0));
	else if(token->kind == IDENT)
		printf("%s", token->text);
	else if(token->kind == STRING)
		printf("\"%s\"", token->text);
	else if(token->kind == PUNCT)
		printf("%s", token->text);
	else if(token->kind == END)
		printf("END");
}

void dump_prim(Expr *expr)
{
	if(expr->prim == INTEGER)
		printf("%lu", expr->val);
	else if(expr->prim == FLOAT)
		printf("%s", d2s(expr->fval, 0));
	else if(expr->prim == IDENT)
		printf("%s", expr->name);
}

void dump_chain(Expr *chain, int braced)
{
	if(braced) printf("(");
	
	if(chain->left->kind == CHAIN && chain->left->tier == chain->tier)
		dump_chain(chain->left, 0);
	else
		dump_expr(chain->left);
	
	printf(" %s ", puncts[chain->op]);
	dump_expr(chain->right);
	if(braced) printf(")");
}

void dump_expr(Expr *expr)
{
	if(expr == 0)
		printf("<null>");
	else if(expr->kind == PRIM)
		dump_prim(expr);
	else if(expr->kind == UNARY) {
		printf("(");
		printf("%s", puncts[expr->op]);
		dump_expr(expr->child);
		printf(")");
	}
	else if(expr->kind == CALL) {
		printf("%s", expr->name);
		printf("(");
		
		for(Expr *arg = expr->child; arg; arg = arg->next) {
			dump_expr(arg);
			
			if(arg->next)
				printf(", ");
		}
		
		printf(")");
	}
	else if(expr->kind == PTR) {
		printf(">");
		dump_expr(expr->child);
	}
	else if(expr->kind == DEREF) {
		printf("(<");
		dump_expr(expr->child);
		printf(")");
	}
	else if(expr->kind == ADDRESS) {
		printf("@");
		dump_expr(expr->child);
	}
	else if(expr->kind == CHAIN) {
		dump_chain(expr, 1);
	}
	else if(expr->kind == ARRAY) {
		printf("[");
		
		for(Expr *item = expr->child; item; item = item->next) {
			dump_expr(item);
			
			if(item->next)
				printf(", ");
		}
		
		printf("]");
	}
	else if(expr->kind == SUBSCRIPT) {
		dump_expr(expr->left);
		printf("[");
		dump_expr(expr->right);
		printf("]");
	}
	else if(expr->kind == SLICE) {
		dump_expr(expr->left);
		printf("[");
		dump_expr(expr->right);
		printf(":");
		dump_expr(expr->slice_end);
		printf("]");
	}
	else if(expr->kind == MEMBER) {
		dump_expr(expr->left);
		printf(".");
		dump_expr(expr->right);
	}
}

void dump_type(Type *type)
{
	if(type == 0)
		printf("<null>");
	else if(type->kind == PRIMTYPE)
		printf("%s", primtype_names[type->primtype]);
	else if(type->kind == NAMEDTYPE)
		printf("%s", type->name);
	else if(type->kind == STRUCTTYPE) {
		printf("struct<");
		printf("%s", type->name);
		printf(">");
	}
	else if(type->kind == PTRTYPE) {
		printf(">");
		dump_type(type->child);
	}
	else if(type->kind == ARRAYTYPE) {
		printf("[");
		printf("%lu", type->count);
		printf("]");
		dump_type(type->child);
	}
	else if(type->kind == SLICETYPE) {
		printf("[]");
		dump_type(type->child);
	}
}

void dump_stmt(Stmt *stmt)
{
	printf("(%lu:%lu) ", stmt->line, stmt->pos);
	
	if(stmt->kind == ASSIGN) {
		dump_expr(stmt->target);
		printf(" %s ", puncts[stmt->op]);
		dump_expr(stmt->expr);
	}
	else if(stmt->kind == VARDECL) {
		if(stmt->exported)
			printf("export ");
		
		printf("%s", stmt->name);
		printf(" : ");
		dump_type(stmt->type);
		
		if(stmt->expr) {
			printf(" = ");
			dump_expr(stmt->expr);
		}
	}
	else if(stmt->kind == FUNCDECL) {
		if(stmt->exported)
			printf("export ");
		
		printf("func ");
		printf("%s", stmt->name);
		printf("(");
		
		for(Stmt *param = stmt->param; param; param = param->next) {
			dump_stmt(param);
			
			if(param->next)
				printf(", ");
		}
		
		printf(")");
		
		if(stmt->type) {
			printf(" : ");
			dump_type(stmt->type);
		}
		
		printf(" {\n");
		level ++;
		dump_block(stmt->body);
		level --;
		dump_indent();
		printf("}");
	}
	else if(stmt->kind == STRUCTDECL) {
		printf("struct ");
		printf("%s", stmt->name);
		printf(" {\n");
		level ++;
		dump_block(stmt->body);
		level --;
		dump_indent();
		printf("}");
	}
	else if(stmt->kind == CALLSTMT) {
		dump_expr(stmt->expr);
	}
	else if(stmt->kind == PRINT) {
		printf("print ");
		
		for(Expr *item = stmt->expr; item; item = item->next) {
			dump_expr(item);
			
			if(item->next)
				printf(", ");
		}
	}
	else if(stmt->kind == RETURN) {
		printf("return ");
		dump_expr(stmt->expr);
	}
	else if(stmt->kind == IMPORT) {
		printf("import ");
		printf("%s", stmt->name);
	}
	else if(stmt->kind == BREAK)
		printf("break");
	else if(stmt->kind == CONTINUE)
		printf("continue");
	else if(stmt->kind == IFSTMT) {
		printf("if ");
		dump_expr(stmt->expr);
		printf(" {\n");
		level ++;
		dump_block(stmt->body);
		level --;
		dump_indent();
		printf("}");
	}
	else if(stmt->kind == WHILESTMT) {
		printf("while ");
		dump_expr(stmt->expr);
		printf(" {\n");
		level ++;
		dump_block(stmt->body);
		level --;
		dump_indent();
		printf("}");
	}
}

void dump_scope(Scope *scope)
{
	if(scope->import_count) {
		dump_indent();
		printf("imported: ");
		
		for(
			Symbol *symbol = scope->first_import; symbol; symbol = symbol->next
		) {
			printf("%s", symbol->decl->name);
			printf(" ");
		}
		
		printf("\n");
	}
	
	if(scope->count) {
		dump_indent();
		printf("scope: ");
		
		for(Symbol *symbol = scope->first; symbol; symbol = symbol->next) {
			printf("%s", symbol->decl->name);
			printf(" ");
		}
		
		printf("\n");
	}
}

void dump_block(Block *block)
{
	dump_scope(block->scope);
	
	for(Stmt *stmt = block->first; stmt; stmt = stmt->next) {
		dump_indent();
		dump_stmt(stmt);
		printf("\n");
	}
}

void dump_tokens()
{
	printf("\ntoken dump of '%s':\n\n", filename);
	
	for(Token *token = unit->tokens; token->kind != END; token ++) {
		printf("%lu:%lu ", token->line, token->pos);
		dump_token(token);
		printf("\n");
	}
}

void dump_ast()
{
	printf("\nast dump of '%s':\n\n", filename);
	
	dump_block(unit->ast);
}
