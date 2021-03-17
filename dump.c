char *primtype_names[] = {
	"u8", "u16", "u32", "u64",
	"i8", "i16", "i32", "i64",
};

void dump_block(Block *block);

void dump_indent()
{
	for(int i=0; i<level; i++)
		printf("  ");
}

void dump_token(Token *token)
{
	if(token->kind == INTEGER)
		printf("%lu", token->val);
	else if(token->kind == IDENT)
		printf("%s", token->text);
	else if(token->kind == STRING)
		printf("\"%s\"", token->text);
	else if(token->kind == PUNCT)
		printf("%s", token->text);
	else if(token->kind == END)
		printf("END");
}

void dump_expr(Expr *expr)
{
	if(expr == 0)
		printf("<null>");
	else if(expr->kind == PRIM)
		dump_token(expr->prim);
	else if(expr->kind == CHAIN) {
		printf("(");
		dump_expr(expr->left);
		printf(" %s ", expr->op->text);
		dump_expr(expr->right);
		printf(")");
	}
}

void dump_type(Type *type)
{
	if(type == 0)
		printf("<null>");
	else if(type->kind == PRIMTYPE)
		printf("%s", primtype_names[type->primtype]);
}

void dump_stmt(Stmt *stmt)
{
	dump_indent();
	
	if(stmt->kind == ASSIGN) {
		dump_token(stmt->ident);
		printf(" = ");
		dump_expr(stmt->expr);
	}
	else if(stmt->kind == VARDECL) {
		if(stmt->exported)
			printf("export ");
		
		dump_token(stmt->ident);
		printf(" : ");
		dump_type(stmt->type);
		printf(" = ");
		dump_expr(stmt->expr);
	}
	else if(stmt->kind == FUNCDECL) {
		if(stmt->exported)
			printf("export ");
		
		printf("func ");
		dump_token(stmt->ident);
		printf("() {\n");
		level ++;
		dump_block(stmt->body);
		level --;
		dump_indent();
		printf("}");
	}
	else if(stmt->kind == CALL) {
		dump_token(stmt->ident);
		printf("()");
	}
	else if(stmt->kind == PRINT) {
		printf("print ");
		dump_expr(stmt->expr);
	}
	else if(stmt->kind == RETURN) {
		printf("return ");
		dump_expr(stmt->expr);
	}
	else if(stmt->kind == IMPORT) {
		printf("import ");
		dump_token(stmt->string);
	}
	
	printf("\n");
}

void dump_scope(Scope *scope)
{
	if(scope->import_count) {
		dump_indent();
		printf("imported: ");
		
		for(
			Symbol *symbol = scope->first_import; symbol; symbol = symbol->next
		) {
			dump_token(symbol->decl->ident);
			printf(" ");
		}
		
		printf("\n");
	}
	
	if(scope->count) {
		dump_indent();
		printf("scope: ");
		
		for(Symbol *symbol = scope->first; symbol; symbol = symbol->next) {
			dump_token(symbol->decl->ident);
			printf(" ");
		}
		
		printf("\n");
	}
}

void dump_block(Block *block)
{
	dump_scope(block->scope);
	
	for(Stmt *stmt = block->first; stmt; stmt = stmt->next)
		dump_stmt(stmt);
}

void dump_tokens()
{
	printf("\ntoken dump of '%s':\n\n", filename);
	
	for(Token *token = unit->tokens->first; token; token = token->next) {
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
