int level = 0;

void gen_block(Block *block);

void gen_indent()
{
	for(int i=0; i<level; i++)
		printf("  ");
}

void gen_token(Token *token)
{
	if(token->kind == INTEGER)
		printf("%lu", token->val);
	else if(token->kind == IDENT)
		printf("%s", token->text);
}

void gen_expr(Expr *expr)
{
	if(expr == 0)
		printf("0");
	else if(expr->kind == PRIM)
		gen_token(expr->prim);
	else if(expr->kind == CHAIN) {
		printf("(");
		gen_expr(expr->left);
		printf(" %s ", expr->op->text);
		gen_expr(expr->right);
		printf(")");
	}
}

void gen_type(Type *type)
{
	if(type == 0)
		printf("int");
	else if(type->kind == PRIMTYPE)
		printf("%s", type->primtype);
}

void gen_proto(Stmt *stmt)
{
	gen_indent();
	printf("void ");
	gen_token(stmt->ident);
	printf("();\n");
}

void gen_decl(Stmt *stmt)
{
	gen_indent();
	
	if(stmt->kind == VARDECL) {
		gen_type(stmt->type);
		printf(" ");
		gen_token(stmt->ident);
		
		if(stmt->expr && stmt->expr->isconst) {
			printf(" = ");
			gen_expr(stmt->expr);
		}
		
		printf(";");
	}
	else if(stmt->kind == FUNCDECL) {
		printf("void ");
		gen_token(stmt->ident);
		printf("() {\n");
		gen_block(stmt->body);
		gen_indent();
		printf("}");
	}
	
	printf("\n");
}

void gen_stmt(Stmt *stmt)
{
	if(stmt->kind == ASSIGN) {
		gen_indent();
		gen_token(stmt->ident);
		printf(" = ");
		gen_expr(stmt->expr);
		printf(";\n");
	}
	else if(stmt->kind == VARDECL) {
		if(stmt->expr && !stmt->expr->isconst) {
			gen_indent();
			gen_token(stmt->ident);
			printf(" = ");
			gen_expr(stmt->expr);
			printf(";\n");
		}
	}
	else if(stmt->kind == CALL) {
		gen_indent();
		gen_token(stmt->ident);
		printf("();\n");
	}
	else if(stmt->kind == PRINT) {
		gen_indent();
		printf("printf(\"%%lu\\n\", ");
		gen_expr(stmt->expr);
		printf(");\n");
	}
}

void gen_scope(Scope *scope)
{
	for(Symbol *symbol = scope->first; symbol; symbol = symbol->next) {
		Stmt *decl = symbol->decl;
		
		if(decl->kind == VARDECL)
			gen_decl(decl);
	}
	
	for(Symbol *symbol = scope->first; symbol; symbol = symbol->next) {
		Stmt *decl = symbol->decl;
		
		if(decl->kind == FUNCDECL)
			gen_proto(decl);
	}
	
	for(Symbol *symbol = scope->first; symbol; symbol = symbol->next) {
		Stmt *decl = symbol->decl;
		
		if(decl->kind == FUNCDECL)
			gen_decl(decl);
	}
}

void gen_block(Block *block)
{
	level ++;
	gen_scope(block->scope);
	
	for(Stmt *stmt = block->first; stmt; stmt = stmt->next)
		gen_stmt(stmt);
	
	level --;
}

void gen_unit(Block *block)
{
	gen_scope(block->scope);
	printf("int main(int argc, char *argv[]) {\n");
	level ++;
	
	for(Stmt *stmt = block->first; stmt; stmt = stmt->next)
		gen_stmt(stmt);
	
	level --;
	printf("}\n");
}

void generate_code()
{
	gen_unit(unit->ast);
}
