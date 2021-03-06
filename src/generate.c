
void gen_block(Block *block);
void gen_vardecl(Stmt *stmt);
void gen_type(Type *type);
void gen_typename(Type *type);
void gen_typedefs(Scope *scope);

void gen_indent()
{
	for(int i=0; i<level; i++)
		fprintf(cfile, "  ");
}

void gen_ident(char *name)
{
	fprintf(cfile, "i_%s", name);
}

void gen_token(Token *token)
{
	if(token->kind == INTEGER)
		fprintf(cfile, "%luL", token->val);
	else if(token->kind == FLOAT)
		fprintf(cfile, "%s", d2s(token->fval, 0));
	else if(token->kind == IDENT)
		gen_ident(token->text);
}

void gen_str_init_expr(Expr *expr)
{
	fprintf(cfile, "{%lu,\"", expr->length);
	
	for(char *c = expr->text; c < expr->text + expr->length; c++) {
		if(*c == '\n') {
			fprintf(cfile, "\\n");
		}
		else if(*c == '\t') {
			fprintf(cfile, "\\t");
		}
		else {
			fprintf(cfile, "%c", *c);
		}
	}
	
	fprintf(cfile, "\"}");
}

void gen_prim(Expr *expr)
{
	if(expr->prim == INTEGER)
		fprintf(cfile, "%luL", expr->val);
	else if(expr->prim == FLOAT)
		fprintf(cfile, "%s", d2s(expr->fval, 0));
	else if(expr->prim == IDENT)
		gen_ident(expr->name);
	else if(expr->prim == STRING) {
		fprintf(cfile, "(String)");
		gen_str_init_expr(expr);
	}
}

void gen_expr(Expr *expr)
{
	if(expr == 0)
		fprintf(cfile, "0");
	else if(expr->kind == PRIM)
		gen_prim(expr);
	else if(expr->kind == UNARY) {
		fprintf(cfile, "%s", puncts[expr->op]);
		gen_expr(expr->child);
	}
	else if(expr->kind == CALL) {
		gen_ident(expr->name);
		fprintf(cfile, "(");
	
		for(Expr *arg = expr->child; arg; arg = arg->next) {
			gen_expr(arg);
			
			if(arg->next)
				fprintf(cfile, ", ");
		}
		
		fprintf(cfile, ")");
	}
	else if(expr->kind == PTR) {
		fprintf(cfile, "&");
		gen_expr(expr->child);
	}
	else if(expr->kind == DEREF) {
		fprintf(cfile, "(*");
		gen_expr(expr->child);
		fprintf(cfile, ")");
	}
	else if(expr->kind == ADDRESS) {
		fprintf(cfile, "((size_t)");
		gen_expr(expr->child);
		fprintf(cfile, ")");
	}
	else if(expr->kind == ARRAY) {
		fprintf(cfile, "{");
		
		for(Expr *item = expr->child; item; item = item->next) {
			gen_expr(item);
			
			if(item->next)
				fprintf(cfile, ", ");
		}
		
		fprintf(cfile, "}");
	}
	else if(expr->kind == CHAIN) {
		if(expr->tier == RELATIONAL) {
			if(expr->left->type->kind == STRINGTYPE) {
			}
			else {
				gen_expr(expr->left);
				
				if(
					expr->left->kind == CHAIN && expr->left->tier == RELATIONAL
				) {
					fprintf(cfile, " && ");
					gen_expr(expr->left->right);
					fprintf(cfile, " %s ", puncts[expr->op]);
					gen_expr(expr->right);
				}
				else {
					fprintf(cfile, " %s ", puncts[expr->op]);
					gen_expr(expr->right);
				}
			}
		}
		else {
			fprintf(cfile, "(");
			gen_expr(expr->left);
			fprintf(cfile, " %s ", puncts[expr->op]);
			gen_expr(expr->right);
			fprintf(cfile, ")");
		}
	}
	else if(expr->kind == SUBSCRIPT) {
		if(expr->left->type->kind == ARRAYTYPE) {
			gen_expr(expr->left);
			fprintf(cfile, "[");
			gen_expr(expr->right);
			fprintf(cfile, "]");
		}
		else if(expr->left->type->kind == SLICETYPE) {
			fprintf(cfile, "((");
			gen_type(expr->type);
			fprintf(cfile, "*)");
			gen_expr(expr->left);
			fprintf(cfile, ".items)[");
			gen_expr(expr->right);
			fprintf(cfile, "]");
		}
	}
	else if(expr->kind == SLICE) {
		if(expr->left->type->kind == ARRAYTYPE) {
			fprintf(cfile, "(");
			gen_typename(expr->type);
			fprintf(cfile, "){");
			gen_expr(expr->left);
			fprintf(cfile, "+");
			gen_expr(expr->right);
			fprintf(cfile, ",");
			gen_expr(expr->slice_end);
			fprintf(cfile, "-");
			gen_expr(expr->right);
			fprintf(cfile, "}");
		}
		else if(expr->left->type->kind == SLICETYPE) {
			fprintf(cfile, "(");
			gen_typename(expr->left->type);
			fprintf(cfile, "){");
			gen_expr(expr->left);
			fprintf(cfile, ".items+");
			gen_expr(expr->right);
			fprintf(cfile, ",");
			gen_expr(expr->slice_end);
			fprintf(cfile, "-");
			gen_expr(expr->right);
			fprintf(cfile, "}");
		}
	}
	else if(expr->kind == MEMBER) {
		gen_expr(expr->left);
		
		if(
			expr->left->type->kind == SLICETYPE &&
			expr->right->name == kw_length
		) {
			fprintf(cfile, ".length");
		}
		else {
			gen_expr(expr->left);
			fprintf(cfile, ".");
			gen_expr(expr->right);
		}
	}
}

void gen_type(Type *type)
{
	if(type == 0)
		fprintf(cfile, "void");
	else if(type->kind == PTRTYPE) {
		gen_type(type->child);
		fprintf(cfile, "(*");
	}
	else if(type->kind == ARRAYTYPE)
		gen_type(type->child);
	else if(type->kind == SLICETYPE) {
		fprintf(cfile, "struct {");
		gen_type(type->child);
		fprintf(cfile, "*items;size_t length;}");
	}
	else if(type->kind == STRUCTTYPE) {
		fprintf(cfile, "struct ");
		gen_ident(type->name);
	}
	else if(type->kind == STRINGTYPE) {
		fprintf(cfile, "struct String");
	}
	else if(type->kind == PRIMTYPE) {
		if(type->primtype == U8)
			fprintf(cfile, "uint8_t");
		else if(type->primtype == U16)
			fprintf(cfile, "uint16_t");
		else if(type->primtype == U32)
			fprintf(cfile, "uint32_t");
		else if(type->primtype == U64)
			fprintf(cfile, "uint64_t");
		else if(type->primtype == I8)
			fprintf(cfile, "int8_t");
		else if(type->primtype == I16)
			fprintf(cfile, "int16_t");
		else if(type->primtype == I32)
			fprintf(cfile, "int32_t");
		else if(type->primtype == I64)
			fprintf(cfile, "int64_t");
		else if(type->primtype == F32)
			fprintf(cfile, "float");
		else if(type->primtype == F64)
			fprintf(cfile, "double");
	}
}

void gen_type_post(Type *type)
{
	if(type == 0)
		return;
	
	if(type->kind == ARRAYTYPE) {
		fprintf(cfile, "[%lu]", type->count);
		gen_type_post(type->child);
	}
	else if(type->kind == PTRTYPE) {
		fprintf(cfile, ")");
		gen_type_post(type->child);
	}
}

void gen_func_head(Stmt *stmt)
{
	if(!stmt->exported)
		fprintf(cfile, "static ");
	
	gen_type(stmt->type);
	fprintf(cfile, " ");
	gen_ident(stmt->name);
	fprintf(cfile, "(");
	scope = stmt->body->scope;
	
	for(Stmt *param = stmt->param; param; param = param->next) {
		gen_vardecl(param);
		
		if(param->next)
			fprintf(cfile, ", ");
	}
	
	scope = scope->parent;
	fprintf(cfile, ")");
	gen_type_post(stmt->type);
}

void gen_proto(Stmt *stmt)
{
	gen_indent();
	gen_func_head(stmt);
	fprintf(cfile, ";\n");
}

void gen_extern_var(Stmt *decl)
{
	fprintf(cfile, "extern ");
	gen_typename(decl->type);
	fprintf(cfile, " ");
	gen_ident(decl->name);
	fprintf(cfile, ";\n");
}

void gen_vardecl(Stmt *stmt)
{
	if(!stmt->exported && scope->parent == 0)
		fprintf(cfile, "static ");
	
	gen_typename(stmt->type);
	fprintf(cfile, " ");
	gen_ident(stmt->name);
	
	if(stmt->expr && stmt->expr->isconst) {
		fprintf(cfile, " = ");
		
		if(stmt->expr->type->kind == STRINGTYPE) {
			gen_str_init_expr(stmt->expr);
		}
		else {
			gen_expr(stmt->expr);
		}
	}
}

void gen_struct_member(Stmt *stmt)
{
	gen_indent();
	gen_typename(stmt->type);
	fprintf(cfile, " ");
	gen_ident(stmt->name);
	fprintf(cfile, ";\n");
}


void gen_struct_block(Block *block)
{
	level ++;
	scope = block->scope;
	
	for(Stmt *stmt = block->first; stmt; stmt = stmt->next)
		gen_struct_member(stmt);
	
	scope = scope->parent;
	level --;
}

void gen_decl(Stmt *stmt)
{
	gen_indent();
	
	if(stmt->kind == VARDECL) {
		gen_vardecl(stmt);
		fprintf(cfile, ";");
	}
	else if(stmt->kind == FUNCDECL) {
		gen_func_head(stmt);
		fprintf(cfile, " {\n");
		gen_block(stmt->body);
		gen_indent();
		fprintf(cfile, "}");
	}
	else if(stmt->kind == STRUCTDECL) {
		fprintf(cfile, "struct ");
		gen_ident(stmt->name);
		fprintf(cfile, " {\n");
		gen_struct_block(stmt->body);
		gen_indent();
		fprintf(cfile, "};");
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
		else if(type->primtype == F32)
			fprintf(cfile, "f");
		else if(type->primtype == F64)
			fprintf(cfile, "f");
	}
	else if(type->kind == STRINGTYPE) {
		fprintf(cfile, "s");
	}
}

void gen_stmt(Stmt *stmt)
{
	if(stmt->kind == ASSIGN) {
		gen_indent();
		gen_expr(stmt->target);
		fprintf(cfile, " %s ", puncts[stmt->op]);
		gen_expr(stmt->expr);
		fprintf(cfile, ";\n");
	}
	else if(stmt->kind == VARDECL) {
		if(stmt->expr && !stmt->expr->isconst) {
			gen_indent();
			gen_ident(stmt->name);
			fprintf(cfile, " = ");
			gen_expr(stmt->expr);
			fprintf(cfile, ";\n");
		}
	}
	else if(stmt->kind == CALLSTMT) {
		gen_indent();
		gen_expr(stmt->expr);
		fprintf(cfile, ";\n");
	}
	else if(stmt->kind == PRINT) {
		gen_indent();
		fprintf(cfile, "printf(\"");
		
		for(Expr *item = stmt->expr; item; item = item->next) {
			fprintf(cfile, "%%");
			gen_printf_format_spec(item->type);
			
			if(item->next)
				fprintf(cfile, " ");
		}
		
		fprintf(cfile, "\\n\", ");
		
		for(Expr *item = stmt->expr; item; item = item->next) {
			gen_expr(item);
			
			if(item->type->kind == STRINGTYPE)
				fprintf(cfile, ".text");
			
			if(item->next)
				fprintf(cfile, ", ");
		}
		
		fprintf(cfile, ");\n");
	}
	/*
	// alternative serial printing of values
	//
	else if(stmt->kind == PRINT) {
		for(Expr *item = stmt->expr; item; item = item->next) {
			gen_indent();
			fprintf(cfile, "printf(\"%%");
			gen_printf_format_spec(item->type);
			fprintf(cfile, " \", ");
			gen_expr(item);
			fprintf(cfile, ");\n");
		}
		
		gen_indent();
		fprintf(cfile, "printf(\"\\n\");\n");
	}
	*/
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
	else if(stmt->kind == BREAK) {
		gen_indent();
		fprintf(cfile, "break;\n");
	}
	else if(stmt->kind == CONTINUE) {
		gen_indent();
		fprintf(cfile, "continue;\n");
	}
}

void gen_export_define(Stmt *decl)
{
	gen_indent();
	fprintf(cfile, "#define ");
	gen_ident(decl->name);
	fprintf(cfile, " x_%lx_", decl->exporthash);
	gen_ident(decl->name);
	fprintf(cfile, "\n");
}

void gen_export_defines(Symbol *symbols)
{
	for(Symbol *symbol = symbols; symbol; symbol = symbol->next) {
		Stmt *decl = symbol->decl;
		
		if(decl->exported)
			gen_export_define(decl);
	}
}

void gen_func_protos(Symbol *symbols)
{
	for(Symbol *symbol = symbols; symbol; symbol = symbol->next) {
		Stmt *decl = symbol->decl;
		
		if(decl->kind == FUNCDECL)
			gen_proto(decl);
	}
}

void gen_typename(Type *type)
{
	if(type == 0)
		fprintf(cfile, "void");
	else if(type->kind == PTRTYPE) {
		fprintf(cfile, "p_");
		gen_typename(type->child);
	}
	else if(type->kind == ARRAYTYPE) {
		fprintf(cfile, "a_%lu_", type->count);
		gen_typename(type->child);
	}
	else if(type->kind == SLICETYPE) {
		fprintf(cfile, "sl_");
		gen_typename(type->child);
	}
	else if(type->kind == STRUCTTYPE) {
		fprintf(cfile, "st_%s", type->name);
	}
	else if(type->kind == STRINGTYPE) {
		fprintf(cfile, "String");
	}
	else if(type->kind == PRIMTYPE) {
		if(type->primtype == U8)
			fprintf(cfile, "u8");
		else if(type->primtype == U16)
			fprintf(cfile, "u16");
		else if(type->primtype == U32)
			fprintf(cfile, "u32");
		else if(type->primtype == U64)
			fprintf(cfile, "u64");
		else if(type->primtype == I8)
			fprintf(cfile, "i8");
		else if(type->primtype == I16)
			fprintf(cfile, "i16");
		else if(type->primtype == I32)
			fprintf(cfile, "i32");
		else if(type->primtype == I64)
			fprintf(cfile, "i64");
		else if(type->primtype == F32)
			fprintf(cfile, "f32");
		else if(type->primtype == F64)
			fprintf(cfile, "f64");
	}
}

void gen_typedefs(Scope *scope)
{
	for(Type *type = scope->types; type; type = type->next) {
		gen_indent();
		fprintf(cfile, "typedef ");
		gen_type(type);
		fprintf(cfile, " ");
		gen_typename(type);
		gen_type_post(type);
		fprintf(cfile, ";\n");
	}
}

void gen_scope(Scope *scope)
{
	gen_export_defines(scope->first_import);
	gen_export_defines(scope->first);
	gen_typedefs(scope);
	
	for(Symbol *symbol = scope->first; symbol; symbol = symbol->next) {
		Stmt *decl = symbol->decl;
		
		if(decl->kind == STRUCTDECL)
			gen_decl(decl);
	}
	
	gen_func_protos(scope->first_import);
	gen_func_protos(scope->first);
	
	for(Symbol *symbol = scope->first; symbol; symbol = symbol->next) {
		Stmt *decl = symbol->decl;
		
		if(decl->kind == VARDECL && decl->isparam == 0)
			gen_decl(decl);
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
	fprintf(cfile, "#include <stdint.h>\n");
	fprintf(cfile, "#include <string.h>\n");
	fprintf(cfile, "struct String {size_t length;char *text;};\n");
	
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
