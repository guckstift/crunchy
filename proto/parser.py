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
		
		return UnknownType()

class VarDecl:
	def __init__(self, ident, data_type):
		self.ident = ident
		self.data_type = data_type
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return " " * level + f"var {self.ident!r} : {self.data_type!r}"

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

class UnknownType:
	def __repr__(self):
		return "<unknown-type>"
	
	def __eq__(self, other):
		return type(self) is type(other)

class UnknownExpr:
	def __repr__(self):
		return "<unknown-expression>"

class Negate:
	def __init__(self, expr):
		self.expr = expr
	
	def __repr__(self):
		return "-" + repr(self.expr)
	
class PrimType:
	def __init__(self, name):
		self.name = name
	
	def __repr__(self):
		return self.name
	
	def __eq__(self, other):
		return type(self) is type(other) and self.name == other.name

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
			
			if type(stmt) is not VarDecl:
				stmts.append(stmt)
		
		return StmtList(stmts)

	def parse_stmt(scope):
		return parse_var_decl(scope) or parse_assign(scope) or parse_print(scope) or parse_if_stmt(scope)

	def parse_var_decl(scope):
		if not parse_keyword("var"):
			return
		
		ident = parse_ident(scope, True) or throw("expected identifier after var") or next_dummy_ident()
		parse_special(":") or throw("expected : after identifier")
		data_type = parse_data_type() or throw("expected type after :") or UnknownType()
		parse_special(";") or throw("expected ; after type")
		var_decl = VarDecl(ident, data_type)
		scope.declare(var_decl)
		return var_decl

	def parse_assign(scope):
		ident = parse_ident(scope)
		
		if not ident:
			return
		
		ident_data_type = scope.lookup_type(ident)
		parse_special("=") or throw("expected = after identifier")
		expr = parse_expr(scope) or throw("expected expression after =") or UnknownExpr()
		expr_data_type = infer_data_type(expr, scope)
		
		if ident_data_type == UnknownType():
			throw("type of", ident, "is unknown")
		elif expr_data_type == UnknownType():
			throw("type of", expr, "is unknown")
		elif expr_data_type != ident_data_type:
			throw("type mismatch, expected", ident_data_type, "got", expr_data_type)
		
		parse_special(";") or throw("expected ; after expression")
		return Assign(ident, expr)
	
	def parse_print(scope):
		if not parse_keyword("print"):
			return
		
		expr = parse_expr(scope) or throw("expected expression after print") or UnknownExpr()
		parse_special(";") or throw("expected ; after expression")
		return Print(expr)
	
	def parse_if_stmt(scope):
		if not parse_keyword("if"):
			return
		
		cond = parse_expr(scope) or throw("expected condition after if") or UnknownExpr()
		data_type = infer_data_type(cond, scope)
		
		if data_type != PrimType("bool"):
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
	
	def parse_data_type():
		keyword = parse_keyword("number") or parse_keyword("bool") or parse_keyword("string")
		
		if keyword:
			return PrimType(keyword.value)

	def parse_expr(scope):
		if parse_special("-"):
			expr = parse_expr(scope)
			data_type = infer_data_type(expr, scope)
			
			if data_type != PrimType("number"):
				throw("can not negate non-number", expr)
			
			return Negate(expr)
		
		return parse_number() or parse_bool() or parse_string() or parse_ident(scope)

	def parse_ident(scope, in_decl = False):
		ident = parse_token(lexer.Ident)
		
		if not ident:
			return
		
		if not in_decl and not scope.lookup(ident):
			throw("could not find", ident)
		
		return ident

	def parse_number():
		return parse_token(lexer.Number)
	
	def parse_bool():
		return parse_token(lexer.Bool)
	
	def parse_string():
		return parse_token(lexer.String)

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

def infer_data_type(expr, scope):
	if type(expr) is lexer.Number:
		return PrimType("number")
	elif type(expr) is lexer.Bool:
		return PrimType("bool")
	elif type(expr) is lexer.String:
		return PrimType("string")
	elif type(expr) is lexer.Ident:
		return scope.lookup_type(expr)
	elif type(expr) is Negate:
		return PrimType("number")
	
	return UnknownType()
