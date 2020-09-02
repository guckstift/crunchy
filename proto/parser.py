#!/usr/bin/env python3

import error
import lexer

class Body:
	def __init__(self, scope, stmts):
		self.scope = scope
		self.stmts = stmts
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return self.scope.to_str(level) + self.stmts.to_str(level)

class Scope:
	def __init__(self, parent = None):
		self.parent = parent
		self.var_decls = {}
	
	def to_str(self, level = 0):
		res = ""
		
		for name in self.var_decls:
			res += self.var_decls[name].to_str(level) + "\n"
		
		return res
	
	def declare(self, var_decl):
		ident = var_decl.ident
		name = ident.value
		
		if name in self.var_decls:
			error.error(ident.line, name, "is already declared")
		else:
			self.var_decls[name] = var_decl
	
	def lookup(self, ident):
		name = ident.value
		
		if name in self.var_decls:
			return self.var_decls[name]
		
		if self.parent:
			return self.parent.lookup(ident)
	
	def lookup_type(self, ident):
		var_decl = self.lookup(ident)
		
		if var_decl:
			return var_decl.data_type
		
		return UnknownType

class VarDecl:
	def __init__(self, ident, data_type, init = None):
		self.ident = ident
		self.data_type = data_type
		self.init = init
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return " " * level + f"var {self.ident!r} : {self.data_type!r}" + (" = " + repr(self.init) if self.init else "")

class StmtList:
	def __init__(self, stmts):
		self.stmts = stmts
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		res = ""
		
		for stmt in self.stmts:
			res += " " * level + repr(stmt) + "\n"
		
		return res

class Assign:
	def __init__(self, ident, expr):
		self.ident = ident
		self.expr = expr
	
	def __repr__(self):
		return f"{self.ident!r} = {self.expr!r}"

class Print:
	def __init__(self, expr):
		self.expr = expr
	
	def __repr__(self):
		return "print " + repr(self.expr)

class IfStmt:
	def __init__(self, cond, body, else_body = None):
		self.cond = cond
		self.body = body
		self.else_body = else_body
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return (
			" " * level
			+ "if "
			+ repr(self.cond)
			+ " {\n"
			+ self.body.to_str(level + 1)
			+ "}"
			+ (
				" else {\n"
				+ self.else_body.to_str(level + 1)
				+ "}"
				if self.else_body
				else ""
			)
		)

class WhileStmt:
	def __init__(self, cond, body):
		self.cond = cond
		self.body = body
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return (
			" " * level
			+ "while "
			+ repr(self.cond)
			+ " {\n"
			+ self.body.to_str(level + 1)
			+ "}"
		)

class UnknownType:
	def __repr__(self):
		return "<unknown-type>"
	
	def __eq__(self, other):
		return type(self) is type(other)

UnknownType = UnknownType()

class UnknownExpr:
	def __init__(self):
		self.data_type = UnknownType
		self.is_const = True
	
	def __repr__(self):
		return "<unknown-expression>"

UnknownExpr = UnknownExpr()

class Negate:
	def __init__(self, expr):
		self.expr = expr
		self.data_type = expr.data_type
		self.is_const = expr.is_const
	
	def __repr__(self):
		return "-" + repr(self.expr)

class BinOp:
	def __init__(self, left, right, op, data_type):
		self.left = left
		self.right = right
		self.op = op
		self.data_type = data_type
		self.is_const = left.is_const and right.is_const
	
	def __repr__(self):
		return f"({self.left} {self.op.value} {self.right})"

class PrimType:
	def __init__(self, name):
		self.name = name
	
	def __repr__(self):
		return self.name
	
	def __eq__(self, other):
		return type(self) is type(other) and self.name == other.name

IntType = PrimType("int")
BoolType = PrimType("bool")
StringType = PrimType("string")

def parse(tokens):
	begin = tokens
	dummy_ident_count = 0

	def parse_unit():
		unit = parse_body()
		parse_end() or throw("unexpected end of source")
		return unit

	def parse_body(scope = None):
		body_scope = Scope(scope)
		stmts = parse_stmts(body_scope)
		return Body(body_scope, stmts)

	def parse_stmts(scope):
		stmts = []
		
		while True:
			stmt = parse_stmt(scope)
			
			if stmt is None:
				break
			
			if type(stmt) is VarDecl:
				if stmt.init and not stmt.init.is_const:
					stmts.append(Assign(stmt.ident, stmt.init))
					stmt.init = None
			else:
				stmts.append(stmt)
		
		return StmtList(stmts)

	def parse_stmt(scope):
		return (
			parse_var_decl(scope) or parse_assign(scope) or parse_print(scope) or parse_if_stmt(scope)
			or parse_while_stmt(scope)
		)

	def parse_var_decl(scope):
		if not parse_keyword("var"):
			return
		
		ident = parse_ident(scope, True) or throw("expected identifier after var") or next_dummy_ident()
		data_type = None
		init = None
		
		if parse_special(":"):
			data_type = parse_data_type() or throw("expected type after :") or UnknownType
		
		if parse_special("="):
			init = parse_expr(scope)
			init_data_type = init.data_type
			
			if not data_type:
				data_type = init_data_type
			
			if init_data_type != data_type:
				throw("initializer data type must be", data_type, "got", init_data_type)
		
		if not data_type and not init:
			throw("expected at least one of a type specification or an initializer")
			data_type = UnknownType
		
		parse_special(";") or throw("expected ; after type")
		var_decl = VarDecl(ident, data_type, init)
		scope.declare(var_decl)
		ident.data_type = data_type
		return var_decl

	def parse_assign(scope):
		ident = parse_ident(scope)
		
		if not ident:
			return
		
		ident_data_type = ident.data_type
		parse_special("=") or throw("expected = after identifier")
		expr = parse_expr(scope) or throw("expected expression after =") or UnknownExpr
		expr_data_type = expr.data_type
		
		if ident_data_type == UnknownType:
			throw("type of", ident, "is unknown")
		elif expr_data_type == UnknownType:
			throw("type of", expr, "is unknown")
		elif expr_data_type != ident_data_type:
			throw("type mismatch, expected", ident_data_type, "got", expr_data_type)
		
		parse_special(";") or throw("expected ; after expression")
		return Assign(ident, expr)
	
	def parse_print(scope):
		if not parse_keyword("print"):
			return
		
		expr = parse_expr(scope) or throw("expected expression after print") or UnknownExpr
		parse_special(";") or throw("expected ; after expression")
		return Print(expr)
	
	def parse_if_stmt(scope):
		if not parse_keyword("if"):
			return
		
		cond = parse_expr(scope) or throw("expected condition after if") or UnknownExpr
		data_type = cond.data_type
		
		if data_type != BoolType:
			throw("condition", cond, "must be of type bool,", data_type, "given")
		
		parse_special("{") or throw("expected { after condition")
		body = parse_body(scope)
		parse_special("}") or throw("expected } after if-body")
		else_body = None
		
		if parse_keyword("else"):
			parse_special("{") or throw("expected { after else")
			else_body = parse_body(scope)
			parse_special("}") or throw("expected } after else-body")
		
		return IfStmt(cond, body, else_body)
	
	def parse_while_stmt(scope):
		if not parse_keyword("while"):
			return
		
		cond = parse_expr(scope) or throw("expected condition after while") or UnknownExpr
		data_type = cond.data_type
		
		if data_type != BoolType:
			throw("condition", cond, "must be of type bool,", data_type, "given")
		
		parse_special("{") or throw("expected { after condition")
		body = parse_body(scope)
		parse_special("}") or throw("expected } after if-body")
		
		return WhileStmt(cond, body)
	
	def parse_data_type():
		keyword = parse_keyword("int") or parse_keyword("bool") or parse_keyword("string")
		
		if keyword:
			return PrimType(keyword.value)

	def parse_expr(scope):
		return parse_chainop(
			scope, ["<", ">"],
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
			
			right = subparse() or throw("expected right side after", op) or UnknownExpr
			right_type = right.data_type
			binop_type = infer_binop_type(left_type, right_type, op.value)
			
			if binop_type == UnknownType:
				throw("can not combine", left_type, "and", right_type, "with the operator", op)
			
			left = BinOp(left, right, op, binop_type)
			left_type = binop_type
	
	def parse_op(ops):
		for op_name in ops:
			op = parse_special(op_name)
			
			if op:
				return op
	
	def parse_negate(scope):
		if parse_special("-"):
			expr = parse_expr(scope)
			data_type = expr.data_type
			
			if data_type != IntType:
				throw("can not negate non-number", expr)
			
			return Negate(expr)
		
		return parse_atom(scope)
	
	def parse_atom(scope):
		return parse_int() or parse_bool() or parse_string() or parse_ident(scope)
	
	def parse_ident(scope, in_decl = False):
		ident = parse_token(lexer.Ident)
		
		if not ident:
			return
		
		if not in_decl and not scope.lookup(ident):
			throw("could not find", ident)
		
		if not in_decl:
			ident.data_type = scope.lookup_type(ident)
		
		ident.is_const = False
		
		return ident

	def parse_int():
		token = parse_token(lexer.Int)
		
		if token:
			token.data_type = IntType
			token.is_const = True
			return token
	
	def parse_bool():
		token = parse_token(lexer.Bool)
		
		if token:
			token.data_type = BoolType
			token.is_const = True
			return token
	
	def parse_string():
		token = parse_token(lexer.String)
		
		if token:
			token.data_type = StringType
			token.is_const = True
			return token

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
	if op == "+" or op == "-" or op == "*":
		if left_type == IntType and right_type == IntType:
			return IntType
	elif op == "<" or op == ">":
		if left_type == IntType and right_type == IntType:
			return BoolType
	
	return UnknownType

