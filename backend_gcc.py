import subprocess
import error
import parser
import lexer
import ast

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
	code = "#include \"c_lib.h\"\n"
	code += gen_string_buffers(scope)
	code += gen_scope(scope)
	code += "int main(int argc, char **argv) {\n"
	code += gen_stmts(stmts, scope, 1)
	code += gen_cleanup(scope, 1)
	code += INDENT + "debug(\"num left allocs %i\\n\", num_mallocs);\n"
	code += "}\n"
	return code

def gen_body(body, level):
	return (
		  gen_scope(body.scope, level)
		+ gen_stmts(body.stmts, body.scope, level)
		+ gen_cleanup(body.scope, level)
	)

def gen_scope(scope, level = 0):
	res = ""
	
	for decl in scope.var_decls:
		res += INDENT * level + gen_var_decl(scope.var_decls[decl]) + "\n"
	
	for decl in scope.func_decls:
		res += INDENT * level + gen_func_decl(scope.func_decls[decl], level) + "\n"
	
	return res

def gen_string_buffers(scope):
	code = ""
	
	for string_value in scope.root.strings:
		string = scope.root.strings[string_value]
		code += gen_string_buffer_decl(string) + "\n"
	
	return code

def gen_string_buffer_decl(string):
	return (
		"char " + string.internal + "[] = \""
		+ int_to_esc_seq(-1)
		+ int_to_esc_seq(len(string.value))
		+ '" "'
		+ string.value
		+ "\";"
	)

def gen_func_decl(func_decl, level = 0):
	ident = func_decl.ident
	body = func_decl.body
	code = (gen_data_type(func_decl.return_type) if func_decl.return_type else "void") + " " + gen_ident(ident) + "("
	code += gen_params(func_decl.params)
	code += ") {\n"
	code += gen_body(body, level + 1)
	code += INDENT * level + "}"
	return code

def gen_params(params):
	generated_params = [gen_param(p) for p in params]
	return ", ".join(generated_params)

def gen_param(param):
	return gen_data_type(param.data_type) + " " + gen_ident(param.ident)

def gen_var_decl(var_decl):
	res = gen_data_type(var_decl.data_type) + " " + gen_ident(var_decl.ident) + " = "
	
	if var_decl.init:
		res += gen_expr(var_decl.init);
	elif var_decl.data_type == ast.StringType:
		res += "((string*)empty_string)";
	elif var_decl.data_type == ast.FloatType:
		res += "0.0"
	else:
		res += "0"
	
	res += ";"
	return res

def gen_cleanup(scope, level = 0):
	res = ""
	
	for name in scope.var_decls:
		decl = scope.var_decls[name]
		
		if decl.data_type == ast.StringType:
			res += INDENT * level + "string_decref(" + gen_ident(decl.ident) + ");\n"
	
	return res

def gen_cleanup_recursive(scope, level = 0):
	return gen_cleanup(scope, level) + (
		gen_cleanup_recursive(scope.parent, level)
		if scope.env == "block" and scope.parent
		else ""
	)

def gen_data_type(data_type):
	if data_type == ast.IntType:
		return "int"
	if data_type == ast.FloatType:
		return "double"
	if data_type == ast.BoolType:
		return "cr_bool"
	if data_type == ast.StringType:
		return "string*"

def gen_ident(ident):
	return ident.internal

def gen_stmts(stmts, scope, level = 0):
	res = ""
	
	for stmt in stmts.stmts:
		res += INDENT * level + gen_stmt(stmt, scope, level) + "\n"
	
	return res

def gen_stmt(stmt, scope, level = 0):
	if type(stmt) is ast.Assign:
		return gen_assign(stmt)
	elif type(stmt) is ast.Call:
		return gen_call_stmt(stmt)
	elif type(stmt) is ast.Return:
		return gen_return(stmt, scope, level)
	elif type(stmt) is ast.Print:
		return gen_print(stmt, scope, level)
	elif type(stmt) is ast.IfStmt:
		return gen_if_stmt(stmt, scope, level)
	elif type(stmt) is ast.WhileStmt:
		return gen_while_stmt(stmt, scope, level)

def gen_assign(assign):
	if assign.ident.data_type == ast.StringType:
		generated_ident = gen_ident(assign.ident)
		return generated_ident + " = string_assign(" + generated_ident + ", " + gen_expr(assign.expr) + ");"
	else:
		return gen_ident(assign.ident) + " = " + gen_expr(assign.expr) + ";"

def gen_call_stmt(call):
	generated_call = gen_call(call)
	
	if call.data_type == ast.StringType:
		return "string_decref(" + generated_call + ");"
	else:
		return generated_call + ";"
	
def gen_call(call):
	return (
		gen_ident(call.ident) + "(" +
		", ".join(gen_expr(arg) for arg in call.args) + 
		")"
	)

def gen_return(return_stmt, scope, level = 0):
	res = ""
	return_value = return_stmt.expr
	
	if return_value:
		return_type = return_value.data_type
		generated_value = gen_expr(return_value)
		
		if return_type == ast.StringType:
			res += "{string* string_res = string_incref(" + generated_value + ");"
			generated_value = "string_res"
	
	res += "\n" + gen_cleanup_recursive(scope, level)
	
	if return_value and return_type == ast.StringType:
		res += INDENT * level + "string_soft_decref(string_res);\n"
	
	res += INDENT * level + "return" + (" " + generated_value if return_value else "") + ";"
	
	if return_value and return_type == ast.StringType:
		res += "}"
	
	return res

def gen_print(print_stmt, scope, level = 0):
	expr_list = print_stmt.expr_list
	printf_format = []
	printf_args = []
	pre_statement = ""
	post_statement = ""
	string_cache_count = 0
	
	for expr in expr_list:
		data_type = expr.data_type
		
		if data_type == ast.IntType:
			if type(expr) is lexer.Int:
				printf_format.append(repr(expr))
			else:
				printf_format.append("%i")
				printf_args.append(gen_expr(expr))
		elif data_type == ast.FloatType:
			if type(expr) is lexer.Float:
				printf_format.append(repr(expr))
			else:
				printf_format.append("%s")
				printf_args.append("d2s(" + gen_expr(expr) + ", 0)")
		elif data_type == ast.BoolType:
			if type(expr) is lexer.Bool:
				printf_format.append(repr(expr))
			else:
				printf_format.append("%s")
				printf_args.append(gen_expr(expr) + " ? \"true\" : \"false\"")
		elif data_type == ast.StringType:
			if type(expr) is lexer.String:
				printf_format.append(expr.value)
			elif type(expr) is lexer.Ident:
				printf_format.append("%.*s")
				printf_args.append(gen_ident(expr) + "->length")
				printf_args.append(gen_ident(expr) + "->data")
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
		+ ");"
		+ (post_statement + "}" if post_statement else "")
	)

def gen_if_stmt(if_stmt, scope, level = 0):
	return (
		"if(" + gen_expr(if_stmt.cond) + ") {\n"
		+ gen_body(if_stmt.body, level + 1)
		+ INDENT * level + "}"
		+ (
			" else {\n"
			+ gen_body(if_stmt.else_body, level + 1)
			+ INDENT * level + "}"
			if if_stmt.else_body
			else ""
		)
	)

def gen_while_stmt(while_stmt, scope, level = 0):
	return (
		"while(" + gen_expr(while_stmt.cond) + ") {\n"
		+ gen_body(while_stmt.body, level + 1)
		+ INDENT * level + "}"
	)

def gen_expr(expr):
	if type(expr) is lexer.Ident:
		return gen_ident(expr)
	elif type(expr) is lexer.Int:
		return gen_int(expr)
	elif type(expr) is lexer.Float:
		return gen_float(expr)
	elif type(expr) is lexer.Bool:
		return gen_bool(expr)
	elif type(expr) is lexer.String:
		return gen_str(expr)
	elif type(expr) is ast.Cast:
		return gen_cast(expr)
	elif type(expr) is ast.Call:
		return gen_call(expr)
	elif type(expr) is ast.Negate:
		return "- " + gen_expr(expr.expr)
	elif type(expr) is ast.BinOp:
		if expr.op.value == "==" and expr.left.data_type == ast.StringType:
			return "string_equ(" + gen_expr(expr.left) + ", " + gen_expr(expr.right) + ")"
		if expr.op.value == "!=" and expr.left.data_type == ast.StringType:
			return "(string_equ(" + gen_expr(expr.left) + ", " + gen_expr(expr.right) + ") == 0)"
		elif expr.data_type == ast.StringType:
			return "string_concat(" + gen_expr(expr.left) + ", " + gen_expr(expr.right) + ")"
		else:
			return "(" + gen_expr(expr.left) + expr.op.value + gen_expr(expr.right) + ")"
	elif type(expr) is ast.ChainOp:
		if expr.data_type == ast.StringType:
			count = len(expr.operands) + 1
			return (
				"string_concats(" + str(count) + ", " +
				gen_expr(expr.start) + ", " +
				", ".join(gen_expr(operand) for operand in expr.operands) + ")"
			)
		else:
			return "(" + gen_expr(expr.start) + "".join(
				op.value + gen_expr(operand) for op, operand in zip(expr.ops, expr.operands)
			) + ")"
	
	error.error(0, "can not generate expression", expr)
	return "0"

def gen_int(integer):
	return str(integer.value)
	
def gen_float(number):
	return str(number.value)

def gen_bool(boolean):
	return "1" if boolean.value else "0"

def gen_str(expr):
	return "((string*)" + expr.internal + ")"

def gen_cast(cast):
	if cast.expr.data_type == ast.IntType:
		if cast.target_type == ast.StringType:
			return "int_to_string(" + gen_expr(cast.expr) + ")"
		elif cast.target_type == ast.FloatType:
			return "((double)" + gen_expr(cast.expr) + ")";
	elif cast.expr.data_type == ast.FloatType:
		if cast.target_type == ast.StringType:
			return "float_to_string(" + gen_expr(cast.expr) + ")"
	elif cast.expr.data_type == ast.BoolType:
		if cast.target_type == ast.IntType:
			return gen_expr(cast.expr)
		elif cast.target_type == ast.FloatType:
			return "((double)" + gen_expr(cast.expr) + ")";
		elif cast.target_type == ast.StringType:
			return "(" + gen_expr(cast.expr) + " ? ((string*)true_string) : ((string*)false_string))"
	
	error.error(0, "can not generate cast", cast)
	return "((" + gen_data_type(cast.target_type) + ")" + gen_expr(cast.expr) + ")"

def compile_code(src_name, code):
	target_name = src_name + ".c"
	fs = open(target_name, "w")
	fs.write(code)
	fs.close()
	exe_name = src_name + ".exe"
	subprocess.run(["gcc", "-I.", "-ansi", "-pedantic", "-o", exe_name, target_name, "c_lib.c", "d2s.c"], check=True)

