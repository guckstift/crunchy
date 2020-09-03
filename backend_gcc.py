import subprocess
import parser
import lexer

c_lib = open("c_lib.c", "r").read()
INDENT = "\t"

def int_to_esc_seq(val):
	s = ""
	
	for i in range(4):
		b = val & 0xff
		s += "\\x" + "{:02X}".format(b)
		val >>= 8
	
	return s

def gen_code(tree):
	scope = tree.scope
	stmts = tree.stmts
	code = c_lib + "\n"
	code += gen_string_buffers(scope)
	code += gen_scope(scope)
	code += "int main(int argc, char **argv) {\n"
	code += gen_stmts(stmts, scope, 1)
	code += gen_cleanup(scope, 1)
	code += "}\n"
	return code

def gen_string_buffers(scope):
	code = ""
	
	for string in scope.root.strings:
		code += gen_string_buffer_decl(scope.root.strings[string]) + "\n"
	
	return code

def gen_scope(scope, level = 0):
	res = ""
	
	for decl in scope.var_decls:
		res += INDENT * level + gen_var_decl(scope.var_decls[decl]) + "\n"
	
	return res

def gen_string_buffer_decl(string):
	return (
		"char " + string.internal + "_buf[] = \""
		+ int_to_esc_seq(1)
		+ int_to_esc_seq(len(string.value))
		+ '" "'
		+ string.value
		+ "\";"
	)

def gen_var_decl(var_decl):
	res = gen_data_type(var_decl.data_type) + " " + gen_ident(var_decl.ident) + " = "
	
	if var_decl.init:
		res += gen_expr(var_decl.init);
	else:
		res += "0";
	
	res += ";"
	return res

def gen_cleanup(scope, level = 0):
	res = ""
	
	for name in scope.var_decls:
		decl = scope.var_decls[name]
		
		if decl.data_type == parser.StringType:
			res += INDENT * level + "string_decref(" + gen_ident(decl.ident) + ");\n"
	
	return res

def gen_data_type(data_type):
	if data_type == parser.IntType:
		return "int"
	if data_type == parser.BoolType:
		return "unsigned char"
	if data_type == parser.StringType:
		return "string*"

def gen_ident(ident):
	return ident.internal

def gen_stmts(stmts, scope, level = 0):
	res = ""
	
	for stmt in stmts.stmts:
		res += INDENT * level + gen_stmt(stmt, scope, level) + "\n"
	
	return res

def gen_stmt(stmt, scope, level = 0):
	if type(stmt) is parser.Assign:
		return gen_assign(stmt)
	elif type(stmt) is parser.Print:
		return gen_print(stmt, scope, level)
	elif type(stmt) is parser.IfStmt:
		return gen_if_stmt(stmt, scope)
	elif type(stmt) is parser.WhileStmt:
		return gen_while_stmt(stmt, scope)

def gen_assign(assign):
	if assign.ident.data_type == parser.StringType:
		generated_ident = gen_ident(assign.ident)
		return generated_ident + " = string_assign(" + generated_ident + ", " + gen_expr(assign.expr) + ");"
	else:
		return gen_ident(assign.ident) + " = " + gen_expr(assign.expr) + ";"

def gen_print(print_stmt, scope, level = 0):
	expr_list = print_stmt.expr_list
	printf_format = []
	printf_args = []
	pre_statement = ""
	post_statement = ""
	string_cache_count = 0
	
	for expr in expr_list:
		data_type = expr.data_type
		
		if data_type == parser.IntType:
			if type(expr) is lexer.Int:
				printf_format.append(repr(expr))
			else:
				printf_format.append("%i")
				printf_args.append(gen_expr(expr))
		elif data_type == parser.BoolType:
			if type(expr) is lexer.Bool:
				printf_format.append(repr(expr))
			else:
				printf_format.append("%s")
				printf_args.append(gen_expr(expr) + " ? \"true\" : \"false\"")
		elif data_type == parser.StringType:
			if type(expr) is lexer.String:
				printf_format.append(expr.value)
			else:
				generated_expr = gen_expr(expr)
				string_cache = "c" + str(string_cache_count)
				string_cache_count += 1
				pre_statement += "string* " + string_cache + " = string_incref(" + generated_expr + ");\n"
				pre_statement += INDENT * level
				printf_format.append("%.*s")
				printf_args.append(string_cache + "->length")
				printf_args.append(string_cache + "->data")
				post_statement += "\n" + INDENT * level + "string_decref(" + string_cache + ");"
	
	return (
		("{" + pre_statement if pre_statement else "")
		+ "printf(\"" + " ".join(printf_format) + "\\n\""
		+ (", " + ", ".join(printf_args) if len(printf_args) > 0 else "")
		+ "); "
		+ (post_statement + "}" if post_statement else "")
	)

def gen_if_stmt(if_stmt, scope, level = 0):
	return (
		INDENT * level
		+ "if("
		+ gen_expr(if_stmt.cond)
		+ ") {\n"
		+ gen_body(if_stmt.body, level + 1)
		+ INDENT * (level + 1)
		+ "}"
		+ (
			" else {\n"
			+ gen_body(if_stmt.else_body, level + 1)
			+ INDENT * (level + 1)
			+ "}"
			if if_stmt.else_body
			else ""
		)
	)

def gen_while_stmt(while_stmt, scope, level = 0):
	return (
		INDENT * level
		+ "while("
		+ gen_expr(while_stmt.cond)
		+ ") {\n"
		+ gen_body(while_stmt.body, level + 1)
		+ INDENT * (level + 1)
		+ "}"
	)

def gen_body(body, level):
	return gen_scope(body.scope, level + 1) + gen_stmts(body.stmts, body.scope, level + 1)

def gen_expr(expr):
	if type(expr) is lexer.Ident:
		return gen_ident(expr)
	elif type(expr) is lexer.Int:
		return gen_int(expr)
	elif type(expr) is lexer.Bool:
		return gen_bool(expr)
	elif type(expr) is lexer.String:
		return gen_str(expr)
	elif type(expr) is parser.Negate:
		return "- " + gen_expr(expr.expr)
	elif type(expr) is parser.BinOp:
		if expr.data_type == parser.StringType:
			return "string_concat(" + gen_expr(expr.left) + ", " + gen_expr(expr.right) + ")"
		else:
			return "(" + gen_expr(expr.left) + expr.op.value + gen_expr(expr.right) + ")"

def gen_int(integer):
	return str(integer.value)

def gen_bool(boolean):
	return "1" if boolean.value else "0"

def gen_str(expr):
	return "((string*)" + expr.internal + "_buf)"

def compile_code(src_name, code):
	target_name = src_name + ".c"
	fs = open(target_name, "w")
	fs.write(code)
	fs.close()
	exe_name = src_name + ".exe"
	subprocess.run(["gcc", "-ansi", "-pedantic", "-o", exe_name, target_name], check=True)

