
void gen_block(Block *block);

void gen_indent()
{
	for(int i=0; i<level; i++)
		fprintf(cfile, "  ");
}

void gen_token(Token *token)
{
	if(token->kind == INTEGER)
		fprintf(cfile, "%luL", token->val);
	else if(token->kind == IDENT)
		fprintf(cfile, "id_%s", token->text);
}

void gen_expr(Expr *expr)
{
	if(expr == 0)
		fprintf(cfile, "0");
	else if(expr->kind == PRIM)
		gen_token(expr->prim);
	else if(expr->kind == CHAIN) {
		fprintf(cfile, "(");
		gen_expr(expr->left);
		fprintf(cfile, " %s ", expr->op->text);
		gen_expr(expr->right);
		fprintf(cfile, ")");
	}
}

void gen_type(Type *type)
{
	if(type == 0)
		fprintf(cfile, "void");
	else if(type->kind == PRIMTYPE) {
		if(type->primtype == U8)
			fprintf(cfile, "unsigned char");
		else if(type->primtype == U16)
			fprintf(cfile, "unsigned short");
		else if(type->primtype == U32)
			fprintf(cfile, "unsigned int");
		else if(type->primtype == U64)
			fprintf(cfile, "unsigned long");
		else if(type->primtype == I8)
			fprintf(cfile, "char");
		else if(type->primtype == I16)
			fprintf(cfile, "short");
		else if(type->primtype == I32)
			fprintf(cfile, "int");
		else if(type->primtype == I64)
			fprintf(cfile, "long");
	}
}

void gen_proto(Stmt *stmt)
{
	gen_indent();
	
	if(!stmt->exported)
		fprintf(cfile, "static ");
	
	gen_type(stmt->type);
	fprintf(cfile, " ");
	gen_token(stmt->ident);
	fprintf(cfile, "();\n");
}

void gen_extern_var(Stmt *decl)
{
	fprintf(cfile, "extern ");
	gen_type(decl->type);
	fprintf(cfile, " ");
	gen_token(decl->ident);
	fprintf(cfile, ";\n");
}

void gen_decl(Stmt *stmt)
{
	gen_indent();
	
	if(stmt->kind == VARDECL) {
		if(!stmt->exported && scope->parent == 0)
			fprintf(cfile, "static ");
		
		gen_type(stmt->type);
		fprintf(cfile, " ");
		gen_token(stmt->ident);
		
		if(stmt->expr && stmt->expr->isconst) {
			fprintf(cfile, " = ");
			gen_expr(stmt->expr);
		}
		
		fprintf(cfile, ";");
	}
	else if(stmt->kind == FUNCDECL) {
		if(!stmt->exported)
			fprintf(cfile, "static ");
		
		gen_type(stmt->type);
		fprintf(cfile, " ");
		gen_token(stmt->ident);
		fprintf(cfile, "() {\n");
		gen_block(stmt->body);
		gen_indent();
		fprintf(cfile, "}");
	}
	
	fprintf(cfile, "\n");
}

void gen_printf_format_spec(Type *type)
{
	if(type->kind == PRIMTYPE) {
		if(type->primtype == U8)
			fprintf(cfile, "u");
		else if(type->primtype == U16)
			fprintf(cfile, "u");
		else if(type->primtype == U32)
			fprintf(cfile, "u");
		else if(type->primtype == U64)
			fprintf(cfile, "lu");
		else if(type->primtype == I8)
			fprintf(cfile, "i");
		else if(type->primtype == I16)
			fprintf(cfile, "i");
		else if(type->primtype == I32)
			fprintf(cfile, "i");
		else if(type->primtype == I64)
			fprintf(cfile, "li");
	}
}

void gen_stmt(Stmt *stmt)
{
	if(stmt->kind == ASSIGN) {
		gen_indent();
		gen_token(stmt->ident);
		fprintf(cfile, " = ");
		gen_expr(stmt->expr);
		fprintf(cfile, ";\n");
	}
	else if(stmt->kind == VARDECL) {
		if(stmt->expr && !stmt->expr->isconst) {
			gen_indent();
			gen_token(stmt->ident);
			fprintf(cfile, " = ");
			gen_expr(stmt->expr);
			fprintf(cfile, ";\n");
		}
	}
	else if(stmt->kind == CALL) {
		gen_indent();
		gen_token(stmt->ident);
		fprintf(cfile, "();\n");
	}
	else if(stmt->kind == PRINT) {
		gen_indent();
		fprintf(cfile, "printf(\"%%");
		gen_printf_format_spec(stmt->expr->type);
		fprintf(cfile, "\\n\", ");
		gen_expr(stmt->expr);
		fprintf(cfile, ");\n");
	}
	else if(stmt->kind == RETURN) {
		gen_indent();
		fprintf(cfile, "return ");
		
		if(stmt->expr)
			gen_expr(stmt->expr);
		
		fprintf(cfile, ";\n");
	}
	else if(stmt->kind == IMPORT) {
		gen_indent();
		fprintf(cfile, "main_%lx(argc, argv);\n", stmt->unit->hash);
	}
	else if(stmt->kind == IFSTMT) {
		gen_indent();
		fprintf(cfile, "if(");
		gen_expr(stmt->expr);
		fprintf(cfile, ") {\n");
		gen_block(stmt->body);
		gen_indent();
		fprintf(cfile, "}\n");
	}
	else if(stmt->kind == WHILESTMT) {
		gen_indent();
		fprintf(cfile, "while(");
		gen_expr(stmt->expr);
		fprintf(cfile, ") {\n");
		gen_block(stmt->body);
		gen_indent();
		fprintf(cfile, "}\n");
	}
}

void gen_export_define(Stmt *decl)
{
	gen_indent();
	fprintf(cfile, "#define ");
	gen_token(decl->ident);
	fprintf(cfile, " ex_");
	fprintf(cfile, "%lx_", decl->exporthash);
	gen_token(decl->ident);
	fprintf(cfile, "\n");
}

void gen_scope(Scope *scope)
{
	for(Symbol *symbol = scope->first_import; symbol; symbol = symbol->next) {
		Stmt *decl = symbol->decl;
		
		if(decl->exported)
			gen_export_define(decl);
	}
	
	for(Symbol *symbol = scope->first_import; symbol; symbol = symbol->next) {
		Stmt *decl = symbol->decl;
		
		if(decl->kind == FUNCDECL)
			gen_proto(decl);
	}
	
	for(Symbol *symbol = scope->first_import; symbol; symbol = symbol->next) {
		Stmt *decl = symbol->decl;
		
		if(decl->kind == VARDECL)
			gen_extern_var(decl);
	}
	
	for(Symbol *symbol = scope->first; symbol; symbol = symbol->next) {
		Stmt *decl = symbol->decl;
		
		if(decl->exported)
			gen_export_define(decl);
	}
	
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
	scope = block->scope;
	gen_scope(block->scope);
	
	for(Stmt *stmt = block->first; stmt; stmt = stmt->next)
		gen_stmt(stmt);
	
	scope = scope->parent;
	level --;
}

void gen_unit(Unit *unit)
{
	Block *block = unit->ast;
	scope = block->scope;
	fprintf(cfile, "#include <stdio.h>\n");
	
	gen_scope(block->scope);
	fprintf(cfile, "int main_%lx(int argc, char *argv[]) {\n", unit->hash);
	fprintf(cfile, "  static int initialized = 0;\n");
	fprintf(cfile, "  if(initialized) return 0;\n");
	fprintf(cfile, "  initialized = 1;\n");
	level ++;
	
	for(Stmt *stmt = block->first; stmt; stmt = stmt->next)
		gen_stmt(stmt);
	
	scope = scope->parent;
	fprintf(cfile, "  return 0;\n");
	level --;
	fprintf(cfile, "}\n");
	fprintf(cfile, "#ifdef CRUNCHY_MAIN\n");
	fprintf(cfile, "int main(int argc, char *argv[]) {\n");
	fprintf(cfile, "  return main_%lx(argc, argv);\n", unit->hash);
	fprintf(cfile, "}\n");
	fprintf(cfile, "#endif\n");
}

void generate_code()
{
	scope = 0;
	cfile = fopen(unit->cpath, "wb");
	gen_unit(unit);
	fclose(cfile);
}
