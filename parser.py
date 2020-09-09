#!/usr/bin/env python3

import error
import lexer
import ast

empty_string = lexer.String(0, 0, "")
empty_string.internal = "empty_string"
false_string = lexer.String(0, 0, "false")
false_string.internal = "false_string"
true_string = lexer.String(0, 0, "true")
true_string.internal = "true_string"

def parse(tokens):
	parser = Parser(tokens)
	return parser.unit()

class Parser:
	def __init__(self, tokens):
		self.cur_scope = None
		self.scope_stack = []
		self.tokens = tokens
		self.begin = tokens
		self.dummy_ident_count = 0
	
	def enter_scope(self, env = "global", return_type = None):
		self.scope_stack.append(self.cur_scope)
		self.cur_scope = ast.Scope(self.cur_scope, env, return_type)
		return self.cur_scope
	
	def leave_scope(self):
		self.cur_scope = self.scope_stack.pop()
	
	def next_dummy_ident(self):
		self.dummy_ident_count += 1
		return lexer.Ident(0, 0, "@" + str(self.dummy_ident_count - 1))
	
	def unit(self):
		unit = self.body()
		self.end() or self.throw("unexpected end of source")
		return unit
	
	def body(self, env = "global", return_type = None):
		body_scope = self.enter_scope(env, return_type)
		body_scope.add_string(empty_string)
		body_scope.add_string(false_string)
		body_scope.add_string(true_string)
		stmts = self.stmts()
		self.leave_scope()
		return ast.Body(body_scope, stmts)
	
	def stmts(self):
		stmts = []
		
		while True:
			stmt = self.stmt()
			
			if stmt is None:
				break
			
			if type(stmt) is ast.VarDecl:
				if stmt.init and not stmt.init.is_const:
					stmts.append(ast.Assign(stmt.ident, stmt.init))
					stmt.init = None
			elif type(stmt) is not ast.FuncDecl:
				stmts.append(stmt)
		
		return ast.StmtList(stmts)
	
	def stmt(self):
		return (
			self.var_decl() or self.func_decl() or
			self.assign() or self.call_stmt() or
			self.return_stmt() or
			self.print() or self.if_stmt() or self.while_stmt()
		)
	
	def func_decl(self):
		if not self.keyword("func"):
			return
		
		ident = self.ident(True) or self.throw("expected identifier after func") or self.next_dummy_ident()
		self.special("(") or self.throw("expected ( after function name")
		params = self.params()
		self.special(")") or self.throw("expected ) after parameter list")
		return_type = ast.VoidType
		
		if self.special(":"):
			return_type = self.data_type() or self.throw("expected type after :")
		
		body = self.expect_block_body("func", return_type)
		
		if return_type and not body.scope.has_toplevel_return:
			self.throw("this function should return a value of type", return_type, "in its outermost scope")
		
		func_decl = ast.FuncDecl(ident, params, return_type, body)
		self.cur_scope.declare(func_decl)
		ident.data_type = func_decl.data_type
		return func_decl
	
	def params(self):
		params = []
		param = self.param()
		
		if not param:
			return []
		
		while True:
			params.append(param)
			
			if not self.special(","):
				break
			
			param = self.param()
			
			if not param:
				self.throw("expected parameter declaration after ,")
				break
		
		return params
		
	def param(self):
		ident = self.ident(True)
		
		if not ident:
			return
		
		self.special(":") or self.throw("expected : after parameter name")
		data_type = self.data_type() or self.throw("expected type after :") or ast.UnknownType
		param = ast.Param(ident, data_type)
		self.cur_scope.declare(param)
		return param
	
	def var_decl(self):
		if not self.keyword("var"):
			return
		
		ident = self.ident(True) or self.throw("expected identifier after var") or self.next_dummy_ident()
		data_type = None
		init = None
		
		if self.special(":"):
			data_type = self.data_type() or self.throw("expected type after :") or ast.UnknownType
		
		if self.special("="):
			init = self.expr()
			init_data_type = init.data_type
			
			if type(init_data_type) is ast.FuncType:
				self.throw("can not initialize", ident, "with function", init)
			
			if not data_type:
				data_type = init_data_type
			
			if init_data_type and init_data_type != data_type:
				init = self.cast_value(init, data_type)
		
		if not data_type and not init:
			self.throw("expected at least one of a type specification or an initializer")
			data_type = ast.UnknownType
		
		self.special(";") or self.throw("expected ; after type")
		var_decl = ast.VarDecl(ident, data_type, init)
		self.cur_scope.declare(var_decl)
		ident.data_type = data_type
		return var_decl
	
	def assign(self):
		self.backup()
		ident = self.ident()
		
		if not ident:
			return
		
		if not self.special("="):
			self.restore()
			return
		
		self.check_ident(ident)
		ident_data_type = ident.data_type
		
		if type(ident_data_type) is ast.FuncType:
			self.throw("can not assign to function", ident)
		
		expr = self.expr() or self.throw("expected expression after =") or ast.UnknownExpr
		expr_data_type = expr.data_type
		
		if type(expr_data_type) is ast.FuncType:
			self.throw("can not assign function", expr, "to variable", ident)
		elif expr_data_type == ast.VoidType:
			self.throw("right-hand side expression is of type void")
		
		if (
			ident_data_type and type(ident_data_type) is not ast.FuncType and
			expr_data_type and type(expr_data_type) is not ast.FuncType and
			expr_data_type != ident_data_type
		):
			expr = self.cast_value(expr, ident_data_type)
		
		self.special(";") or self.throw("expected ; after expression")
		return ast.Assign(ident, expr)
	
	def call_stmt(self):
		call = self.call()
		
		if not call:
			return
		
		self.special(";") or self.throw("expected ; after )")
		return call
	
	def call(self):
		self.backup()
		ident = self.ident()
		
		if not ident:
			return
		
		if not self.special("("):
			self.restore()
			return
		
		if self.check_ident(ident) and type(ident.data_type) is not ast.FuncType:
			self.throw(ident, "is not a function")
		
		func_decl = self.cur_scope.lookup(ident)
		args = self.args(func_decl)
		
		self.special(")") or self.throw("expected ) after (")
		return ast.Call(ident, args)
	
	def args(self, func_decl):
		args = []
		arg = self.expr()
		params = func_decl.params
		param_count = len(params)
		i = 0
		
		if not arg:
			return []
		
		while True:
			if i < param_count:
				param = params[i]
				
				if param.data_type != arg.data_type:
					self.throw("type of parameter", i + 1, "is", param.data_type, ", got", arg.data_type)
				
			args.append(arg)
			
			if not self.special(","):
				break
			
			arg = self.expr()
			
			if not arg:
				self.throw("expected argument after ,")
				break
			
			i += 1
		
		if len(args) != param_count:
			self.throw("expected", param_count, "arguments, got", len(args))
		
		return args
	
	def return_stmt(self):
		if not self.keyword("return"):
			return
		
		expr = self.expr()
		self.special(";") or self.throw("expected ; after return statement")
		scope = self.cur_scope
		func_scope = scope.last_func_ancestor or self.throw("can not return from outside a function")
		
		if expr and not scope.return_type:
			self.throw("this function should not return a value")
		elif not expr and scope.return_type:
			self.throw("this function should return a value")
		elif expr and expr.data_type != scope.return_type:
			expr = self.cast_value(expr, scope.return_type)
		
		if scope.env == "func":
			scope.has_toplevel_return = True
		
		return ast.Return(expr)
	
	def print(self):
		if not self.keyword("print"):
			return
		
		expr = self.expr() or self.throw("expected expression after print") or ast.UnknownExpr
		expr_list = [expr]
		
		while self.special(","):
			expr = self.expr() or self.throw("expected expression after ,") or ast.UnknownExpr
			expr_list.append(expr)
		
		self.special(";") or self.throw("expected ; after expression")
		return ast.Print(expr_list)
	
	def expect_block_body(self, env = "block", return_type = None):
		self.special("{") or self.throw("expected { before body")
		body = self.body(env, return_type)
		self.special("}") or self.throw("expected } after body")
		return body
	
	def if_stmt(self):
		if not self.keyword("if"):
			return
		
		cond = self.expr() or self.throw("expected condition after if") or ast.UnknownExpr
		data_type = cond.data_type
		
		if cond != ast.UnknownExpr and data_type != ast.BoolType:
			self.throw("condition", cond, "must be of type bool,", data_type, "given")
		
		body = self.expect_block_body()
		else_body = None
		
		if self.keyword("else"):
			else_body = self.expect_block_body()
		
		return ast.IfStmt(cond, body, else_body)
	
	def while_stmt(self):
		if not self.keyword("while"):
			return
		
		cond = self.expr() or self.throw("expected condition after while") or ast.UnknownExpr
		data_type = cond.data_type
		
		if cond != ast.UnknownExpr and data_type != ast.BoolType:
			self.throw("condition", cond, "must be of type bool,", data_type, "given")
		
		body = self.expect_block_body()
		
		return ast.WhileStmt(cond, body)
	
	def data_type(self):
		keyword = self.keyword("int") or self.keyword("bool") or self.keyword("string") or self.keyword("float")
		
		if keyword:
			return ast.PrimType(keyword.value)
	
	def expr(self):
		return self.chainop(
			["==", "!=", "<=", ">=", "<", ">"],
			lambda: self.chainop(
				["+", "-"],
				lambda: self.chainop(
					["*"],
					lambda: self.negate(),
				),
			),
		)
	
	def chainop(self, ops, subparse):
		left = subparse()
		
		if not left:
			return
		
		left_type = left.data_type
		
		while True:
			op = self.op(ops)
			
			if not op:
				return left
			
			right = subparse() or self.throw("expected right side after", op) or ast.UnknownExpr
			right_type = right.data_type
			binop_type = self.infer_binop_type(left_type, right_type, op.value)
			
			if left_type and right_type and binop_type == ast.UnknownType:
				self.throw("can not combine", left_type, "and", right_type, "with the operator", op)
			
			left = ast.BinOp(left, right, op, binop_type)
			left_type = binop_type
			self.unify_binop(left)
	
	def op(self, ops):
		for op_name in ops:
			op = self.special(op_name)
			
			if op:
				return op
	
	def negate(self):
		if self.special("-"):
			expr = self.negate()
			data_type = expr.data_type
			
			if data_type != ast.IntType:
				self.throw("can not negate non-number", expr)
			
			return ast.Negate(expr)
		
		return self.call() or self.atom()
	
	def atom(self):
		ident = self.ident()
		
		if ident:
			self.check_ident(ident)
			return ident
		
		return self.int() or self.float() or self.bool() or self.string() or self.ident()
	
	def ident(self, in_decl = False):
		ident = self.token(lexer.Ident)
		
		if ident:
			if not in_decl:
				decl = self.cur_scope.lookup(ident)
				ident.data_type = self.cur_scope.lookup_type(ident)
				
				if decl:
					ident.internal = decl.ident.internal
			
			ident.is_const = False
			return ident
	
	def check_ident(self, ident):
		decl = self.cur_scope.lookup(ident)
		
		if not decl:
			self.throw("could not find", ident)
			return False
		elif ident.data_type == ast.UnknownType:
			self.throw(ident, "was not declared properly")
			return False
		
		return True
	
	def int(self):
		token = self.token(lexer.Int)
		
		if token:
			token.data_type = ast.IntType
			token.is_const = True
			return token
	
	def float(self):
		token = self.token(lexer.Float)
		
		if token:
			token.data_type = ast.FloatType
			token.is_const = True
			return token
	
	def bool(self):
		token = self.token(lexer.Bool)
		
		if token:
			token.data_type = ast.BoolType
			token.is_const = True
			return token
	
	def string(self):
		string = self.token(lexer.String)
		
		if string:
			string.data_type = ast.StringType
			string.is_const = False
			string.internal = self.cur_scope.add_string(string)
			return string
	
	def keyword(self, name):
		return self.token(lexer.Keyword, name)
	
	def special(self, name):
		return self.token(lexer.Special, name)
	
	def end(self):
		return self.token(lexer.End)
	
	def token(self, kind, value = None):
		token = self.tokens[0]
		
		if type(token) is kind and (value is None or token.value == value):
			self.tokens = self.tokens[1:]
			return token
	
	def cast_value(self, expr, target_type):
		expr_type = expr.data_type
		
		if (
			target_type == ast.StringType and (
				expr_type == ast.FloatType or
				expr_type == ast.IntType or
				expr_type == ast.BoolType
			)
		):
			return ast.Cast(expr, target_type, False)
		
		if (
			target_type == ast.FloatType and (
				expr_type == ast.IntType or
				expr_type == ast.BoolType
			) or
			target_type == ast.IntType and (
				expr_type == ast.BoolType
			)
		):
			return ast.Cast(expr, target_type)
		
		self.throw("can not cast", expr_type, "to", target_type)
		return expr
	
	def backup(self):
		self.begin = self.tokens
	
	def restore(self):
		self.tokens = self.begin
	
	def throw(self, *args):
		return error.error(self.tokens[0].line, *args)
	
	def fatal(self, *args):
		return error.fatal(self.tokens[0].line, *args)
	
	def infer_binop_type(self, left_type, right_type, op):
		num_types = [ast.BoolType, ast.IntType, ast.FloatType]
		
		if op == "+" and left_type == right_type == ast.StringType:
			return ast.StringType
		elif op == "+" or op == "-" or op == "*":
			if (
				left_type == ast.FloatType and right_type == ast.IntType or
				left_type == ast.IntType and right_type == ast.FloatType
			):
				return ast.FloatType
			if (
				left_type == ast.FloatType and right_type == ast.BoolType or
				left_type == ast.BoolType and right_type == ast.FloatType
			):
				return ast.FloatType
			if (
				left_type == ast.IntType and right_type == ast.BoolType or
				left_type == ast.BoolType and right_type == ast.IntType
			):
				return ast.IntType
			elif left_type == right_type == ast.BoolType:
				return ast.IntType
			elif left_type == right_type == ast.IntType:
				return ast.IntType
			elif left_type == right_type == ast.FloatType:
				return ast.FloatType
		elif op in ["==", "!=", "<=", ">=", "<", ">"]:
			if left_type in num_types and right_type in num_types:
				return ast.BoolType
			elif op in ["==", "!="] and left_type == ast.StringType and right_type == ast.StringType:
				return ast.BoolType
		
		return ast.UnknownType
	
	def unify_binop(self, binop):
		left_type = binop.left.data_type
		right_type = binop.right.data_type
		op = binop.op.value
		
		if op in ["+", "-", "*", "==", "!=", "<=", ">=", "<", ">"]:
			if left_type == ast.FloatType and right_type == ast.IntType:
				binop.right = ast.Cast(binop.right, ast.FloatType)
			elif left_type == ast.IntType and right_type == ast.FloatType:
				binop.left = ast.Cast(binop.left, ast.FloatType)
			elif left_type == ast.FloatType and right_type == ast.BoolType:
				binop.right = ast.Cast(binop.right, ast.FloatType)
			elif left_type == ast.BoolType and right_type == ast.FloatType:
				binop.left = ast.Cast(binop.left, ast.FloatType)
			elif left_type == ast.IntType and right_type == ast.BoolType:
				binop.right = ast.Cast(binop.right, ast.IntType)
			elif left_type == ast.BoolType and right_type == ast.IntType:
				binop.left = ast.Cast(binop.left, ast.IntType)


