#!/usr/bin/env python3

import error
import lexer
import ast

empty_string = lexer.String(0, 0, "")
empty_string.internal = "empty_string"

def parse(tokens):
	begin = tokens
	dummy_ident_count = 0

	def parse_unit():
		unit = parse_body()
		parse_end() or throw("unexpected end of source")
		return unit

	def parse_body(parent_scope = None, ctx = "global", return_type = None):
		body_scope = ast.Scope(parent_scope, ctx, return_type)
		body_scope.add_string(empty_string)
		stmts = parse_stmts(body_scope)
		return ast.Body(body_scope, stmts)

	def parse_stmts(scope):
		stmts = []
		
		while True:
			stmt = parse_stmt(scope)
			
			if stmt is None:
				break
			
			if type(stmt) is ast.VarDecl:
				if stmt.init and not stmt.init.is_const:
					stmts.append(ast.Assign(stmt.ident, stmt.init))
					stmt.init = None
			elif type(stmt) is not ast.FuncDecl:
				stmts.append(stmt)
		
		return ast.StmtList(stmts)

	def parse_stmt(scope):
		return (
			parse_var_decl(scope) or parse_func_decl(scope) or
			parse_assign(scope) or parse_call_stmt(scope) or
			parse_return(scope) or
			parse_print(scope) or parse_if_stmt(scope) or parse_while_stmt(scope)
		)
	
	def parse_func_decl(scope):
		if not parse_keyword("func"):
			return
		
		ident = parse_ident(scope, True) or throw("expected identifier after func") or next_dummy_ident()
		parse_special("(") or throw("expected ( after function name")
		parse_special(")") or throw("expected ) after (")
		return_type = ast.VoidType
		
		if parse_special(":"):
			return_type = parse_data_type() or throw("expected type after :")
		
		body = expect_block_body(scope, "func", return_type)
		
		if return_type and not body.scope.has_toplevel_return:
			throw("this function should return a value of type", return_type, "in its outermost scope")
		
		func_decl = ast.FuncDecl(ident, return_type, body)
		scope.declare(func_decl)
		ident.data_type = func_decl.data_type
		return func_decl

	def parse_var_decl(scope):
		if not parse_keyword("var"):
			return
		
		ident = parse_ident(scope, True) or throw("expected identifier after var") or next_dummy_ident()
		data_type = None
		init = None
		
		if parse_special(":"):
			data_type = parse_data_type() or throw("expected type after :") or ast.UnknownType
		
		if parse_special("="):
			init = parse_expr(scope)
			init_data_type = init.data_type
			
			if type(init_data_type) is ast.FuncType:
				throw("can not initialize", ident, "with function", init)
			
			if not data_type:
				data_type = init_data_type
			
			if init_data_type and init_data_type != data_type:
				throw("initializer data type must be", data_type, "got", init_data_type)
		
		if not data_type and not init:
			throw("expected at least one of a type specification or an initializer")
			data_type = ast.UnknownType
		
		parse_special(";") or throw("expected ; after type")
		var_decl = ast.VarDecl(ident, data_type, init)
		scope.declare(var_decl)
		ident.data_type = data_type
		return var_decl
	
	def parse_assign(scope):
		backup()
		ident = parse_ident(scope)
		
		if not ident:
			return
		
		if not parse_special("="):
			restore()
			return
		
		check_ident(ident, scope)
		ident_data_type = ident.data_type
		
		if type(ident_data_type) is ast.FuncType:
			throw("can not assign to function", ident)
		
		expr = parse_expr(scope) or throw("expected expression after =") or ast.UnknownExpr
		expr_data_type = expr.data_type
		
		if type(expr_data_type) is ast.FuncType:
			throw("can not assign function", expr, "to variable", ident)
		elif expr_data_type == ast.VoidType:
			throw("right-hand side expression is of type void")
		
		if (
			ident_data_type and type(ident_data_type) is not ast.FuncType and
			expr_data_type and type(expr_data_type) is not ast.FuncType and
			expr_data_type != ident_data_type
		):
			throw("type mismatch, expected", ident_data_type, "got", expr_data_type)
		
		parse_special(";") or throw("expected ; after expression")
		return ast.Assign(ident, expr)
	
	def parse_call_stmt(scope):
		call = parse_call(scope)
		
		if not call:
			return
		
		parse_special(";") or throw("expected ; after )")
		return call
	
	def parse_call(scope):
		backup()
		ident = parse_ident(scope)
		
		if not ident:
			return
		
		if not parse_special("("):
			restore()
			return
		
		if check_ident(ident, scope) and type(ident.data_type) is not ast.FuncType:
			throw(ident, "is not a function")
		
		parse_special(")") or throw("expected ) after (")
		return ast.Call(ident)
	
	def parse_return(scope):
		if not parse_keyword("return"):
			return
		
		expr = parse_expr(scope)
		parse_special(";") or throw("expected ; after return statement")
		func_scope = scope.last_func_ancestor or throw("can not return from outside a function")
		
		if expr and not scope.return_type:
			throw("this function should not return a value")
		elif not expr and scope.return_type:
			throw("this function should return a value")
		elif expr and expr.data_type != scope.return_type:
			throw("must return value of type", scope.return_type, "got", expr.data_type, "instead")
		
		if scope.ctx == "func":
			scope.has_toplevel_return = True
		
		return ast.Return(expr)
	
	def parse_print(scope):
		if not parse_keyword("print"):
			return
		
		expr = parse_expr(scope) or throw("expected expression after print") or ast.UnknownExpr
		expr_list = [expr]
		
		while parse_special(","):
			expr = parse_expr(scope) or throw("expected expression after ,") or ast.UnknownExpr
			expr_list.append(expr)
		
		parse_special(";") or throw("expected ; after expression")
		return ast.Print(expr_list)
	
	def expect_block_body(scope, ctx = "block", return_type = None):
		parse_special("{") or throw("expected { before body")
		body = parse_body(scope, ctx, return_type)
		parse_special("}") or throw("expected } after body")
		return body
	
	def parse_if_stmt(scope):
		if not parse_keyword("if"):
			return
		
		cond = parse_expr(scope) or throw("expected condition after if") or ast.UnknownExpr
		data_type = cond.data_type
		
		if cond != ast.UnknownExpr and data_type != ast.BoolType:
			throw("condition", cond, "must be of type bool,", data_type, "given")
		
		body = expect_block_body(scope)
		else_body = None
		
		if parse_keyword("else"):
			else_body = expect_block_body(scope)
		
		return ast.IfStmt(cond, body, else_body)
	
	def parse_while_stmt(scope):
		if not parse_keyword("while"):
			return
		
		cond = parse_expr(scope) or throw("expected condition after while") or ast.UnknownExpr
		data_type = cond.data_type
		
		if cond != ast.UnknownExpr and data_type != ast.BoolType:
			throw("condition", cond, "must be of type bool,", data_type, "given")
		
		body = expect_block_body(scope)
		
		return ast.WhileStmt(cond, body)
	
	def parse_data_type():
		keyword = parse_keyword("int") or parse_keyword("bool") or parse_keyword("string")
		
		if keyword:
			return ast.PrimType(keyword.value)

	def parse_expr(scope):
		return parse_chainop(
			scope, ["==", "!=", "<=", ">=", "<", ">"],
			lambda: parse_chainop(
				scope, ["+", "-"],
				lambda: parse_chainop(
					scope, ["*"],
					lambda: parse_negate(scope),
				),
			),
		)
	
	def parse_chainop(scope, ops, subparse):
		left = subparse()
		
		if not left:
			return
		
		left_type = left.data_type
		
		while True:
			op = parse_op(ops)
			
			if not op:
				return left
			
			right = subparse() or throw("expected right side after", op) or ast.UnknownExpr
			right_type = right.data_type
			binop_type = infer_binop_type(left_type, right_type, op.value)
			
			if left_type and right_type and binop_type == ast.UnknownType:
				throw("can not combine", left_type, "and", right_type, "with the operator", op)
			
			left = ast.BinOp(left, right, op, binop_type)
			left_type = binop_type
	
	def parse_op(ops):
		for op_name in ops:
			op = parse_special(op_name)
			
			if op:
				return op
	
	def parse_negate(scope):
		if parse_special("-"):
			expr = parse_negate(scope)
			data_type = expr.data_type
			
			if data_type != ast.IntType:
				throw("can not negate non-number", expr)
			
			return ast.Negate(expr)
		
		return parse_call(scope) or parse_atom(scope)
	
	def parse_atom(scope):
		ident = parse_ident(scope)
		
		if ident:
			check_ident(ident, scope)
			return ident
		
		return parse_int() or parse_bool() or parse_string(scope) or parse_ident(scope)
	
	def parse_ident(scope, in_decl = False):
		ident = parse_token(lexer.Ident)
		
		if ident:
			if not in_decl:
				decl = scope.lookup(ident)
				ident.data_type = scope.lookup_type(ident)
				
				if decl:
					ident.internal = decl.ident.internal
			
			ident.is_const = False
			return ident
		
	def check_ident(ident, scope):
		decl = scope.lookup(ident)
		
		if not decl:
			throw("could not find", ident)
			return False
		elif ident.data_type == ast.UnknownType:
			throw(ident, "was not declared properly")
			return False
		
		return True

	def parse_int():
		token = parse_token(lexer.Int)
		
		if token:
			token.data_type = ast.IntType
			token.is_const = True
			return token
	
	def parse_bool():
		token = parse_token(lexer.Bool)
		
		if token:
			token.data_type = ast.BoolType
			token.is_const = True
			return token
	
	def parse_string(scope):
		string = parse_token(lexer.String)
		
		if string:
			string.data_type = ast.StringType
			string.is_const = False
			string.internal = scope.add_string(string)
			return string

	def parse_keyword(name):
		return parse_token(lexer.Keyword, name)

	def parse_special(name):
		return parse_token(lexer.Special, name)
	
	def parse_end():
		return parse_token(lexer.End)
	
	def parse_token(kind, value = None):
		nonlocal tokens
		token = tokens[0]
		
		if type(token) is kind and (value is None or token.value == value):
			tokens = tokens[1:]
			return token
	
	def backup():
		nonlocal begin
		begin = tokens
	
	def restore():
		nonlocal tokens
		tokens = begin
	
	def throw(*args):
		return error.error(tokens[0].line, *args)
	
	def fatal(*args):
		return error.fatal(tokens[0].line, *args)
	
	def next_dummy_ident():
		nonlocal dummy_ident_count
		dummy_ident_count += 1
		return lexer.Ident(0, 0, "@" + str(dummy_ident_count - 1))

	return parse_unit()

def infer_binop_type(left_type, right_type, op):
	if op == "+" and left_type == right_type == ast.StringType:
		return ast.StringType
	elif op == "+" or op == "-" or op == "*":
		if left_type == right_type == ast.IntType:
			return ast.IntType
	elif op in ["==", "!=", "<=", ">=", "<", ">"]:
		if left_type == right_type == ast.IntType:
			return ast.BoolType
	
	return ast.UnknownType

