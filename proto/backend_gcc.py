import subprocess
import parser
import lexer

def gen_code(tree):
	code = "#include <stdio.h>\n"
	code += gen_scope(tree.scope)
	code += "int main(int argc, char **argv) {\n"
	code += gen_stmts(tree.stmts, tree.scope, 1)
	code += "}\n"
	return code

def gen_scope(scope, level = 0):
	res = ""
	
	for decl in scope.var_decls:
		res += " " * level + gen_var_decl(scope.var_decls[decl]) + "\n"
	
	return res

def gen_var_decl(var_decl):
	res = gen_data_type(var_decl.data_type) + " " + gen_ident(var_decl.ident) + " = "
	
	if var_decl.init:
		res += gen_expr(var_decl.init);
	else:
		res += "0";
	
	res += ";"
	return res

def gen_data_type(data_type):
	if data_type == parser.IntType:
		return "int"
	if data_type == parser.BoolType:
		return "unsigned char"
	if data_type == parser.StringType:
		return "char*"

def gen_ident(ident):
	return ident.value

def gen_stmts(stmts, scope, level = 0):
	res = ""
	
	for stmt in stmts.stmts:
		res += " " * level + gen_stmt(stmt, scope) + "\n"
	
	return res

def gen_stmt(stmt, scope):
	if type(stmt) is parser.Assign:
		return gen_assign(stmt)
	elif type(stmt) is parser.Print:
		return gen_print(stmt, scope)
	elif type(stmt) is parser.IfStmt:
		return gen_if_stmt(stmt, scope)
	elif type(stmt) is parser.WhileStmt:
		return gen_while_stmt(stmt, scope)

def gen_assign(assign):
	return gen_ident(assign.ident) + " = " + gen_expr(assign.expr) + ";"

def gen_print(print_stmt, scope):
	expr_list = print_stmt.expr_list
	printf_format = []
	printf_args = []
	
	for expr in expr_list:
		data_type = expr.data_type
		
		if data_type == parser.IntType:
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
				printf_format.append("%s")
				printf_args.append(gen_expr(expr))
	
	return "printf(\"" + " ".join(printf_format) + "\\n\", " + ", ".join(printf_args) + ");"

def gen_if_stmt(if_stmt, scope, level = 0):
	return (
		" " * level
		+ "if("
		+ gen_expr(if_stmt.cond)
		+ ") {\n"
		+ gen_body(if_stmt.body, level + 1)
		+ " " * (level + 1)
		+ "}"
		+ (
			" else {\n"
			+ gen_body(if_stmt.else_body, level + 1)
			+ " " * (level + 1)
			+ "}"
			if if_stmt.else_body
			else ""
		)
	)

def gen_while_stmt(while_stmt, scope, level = 0):
	return (
		" " * level
		+ "while("
		+ gen_expr(while_stmt.cond)
		+ ") {\n"
		+ gen_body(while_stmt.body, level + 1)
		+ " " * (level + 1)
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
		return "(" + gen_expr(expr.left) + expr.op.value + gen_expr(expr.right) + ")"

def gen_int(integer):
	return str(integer.value)

def gen_bool(boolean):
	return "1" if boolean.value else "0"

def gen_str(expr):
	return '"' + expr.value + '"'

def compile_code(src_name, code):
	target_name = src_name + ".c"
	fs = open(target_name, "w")
	fs.write(code)
	fs.close()
	exe_name = src_name + ".exe"
	subprocess.run(["gcc", "-o", exe_name, target_name], check=True)

