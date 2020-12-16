import error

class Body:
	def __init__(self, scope, stmts = None):
		self.scope = scope
		self.stmts = stmts or StmtList()
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return self.scope.to_str(level) + self.stmts.to_str(level)

class Scope:
	def __init__(self, parent = None, env = "global", return_type = None):
		self.parent = parent
		self.env = env
		self.var_decls = {}
		self.func_decls = {}
		self.root = self.parent.root if self.parent else self
		self.is_global = parent is None
		self.last_func_ancestor = None
		self.return_type = None
		self.has_toplevel_return = False
		
		if env == "func":
			self.last_func_ancestor = self
		elif self.parent:
			self.last_func_ancestor = self.parent.last_func_ancestor
		
		if return_type:
			self.return_type = return_type
		elif self.last_func_ancestor:
			self.return_type = self.last_func_ancestor.return_type
		
		if self is self.root:
			self.internal_count = 0
			self.string_count = 0
			self.strings = {}
	
	def next_internal(self):
		internal = "v" + str(self.root.internal_count)
		self.root.internal_count += 1
		return internal
	
	def next_string_internal(self):
		internal = "s" + str(self.root.string_count)
		self.root.string_count += 1
		return internal
	
	def add_string(self, string):
		value = string.value
		
		if value not in self.root.strings:
			internal = self.next_string_internal()
			self.root.strings[value] = string
			return internal
		
		return self.root.strings[value].internal
	
	def to_str(self, level = 0):
		res = " " * level + f"Scope({self.env})\n"
		
		for name in self.var_decls:
			res += self.var_decls[name].to_str(level) + "\n"
		
		for name in self.func_decls:
			res += self.func_decls[name].to_str(level) + "\n"
		
		return res
	
	def declare(self, decl):
		ident = decl.ident
		name = ident.value
		
		if type(decl) is VarDecl or type(decl) is Param:
			decls = self.var_decls
		else:
			decls = self.func_decls
		
		if type(decl) is FuncDecl and not self.is_global:
			error.error(ident.line, "function declaration of ", ident, "is only allowed in global scope")
			return
				
		if name in self.var_decls or name in self.func_decls:
			error.error(ident.line, name, "is already declared")
		else:
			decls[name] = decl
			ident.internal = self.next_internal()
	
	def lookup(self, ident):
		name = ident.value
		
		if name in self.var_decls:
			return self.var_decls[name]
		
		if name in self.func_decls:
			return self.func_decls[name]
		
		if self.parent:
			return self.parent.lookup(ident)
	
	def lookup_type(self, ident):
		decl = self.lookup(ident)
		
		if decl:
			return decl.data_type
		
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

class FuncDecl:
	def __init__(self, ident, params, return_type, body):
		self.ident = ident
		self.params = params
		self.return_type = return_type
		self.body = body
		self.data_type = FuncType(return_type)
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return (
			" " * level + f"func {self.ident!r}(" +
			", ".join(repr(p) for p in self.params) +
			")" +
			(" : " + repr(self.return_type) if self.return_type else "") +
			" {\n" + self.body.to_str(level + 1) + "\n" + " " * level + "}"
		)

class Param:
	def __init__(self, ident, data_type):
		self.ident = ident
		self.data_type = data_type
		self.init = None
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return f"{self.ident} : {self.data_type}"

class StmtList:
	def __init__(self, stmts = None):
		self.stmts = stmts or []
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		res = ""
		
		for stmt in self.stmts:
			res += " " * level + stmt.to_str(level) + "\n"
		
		return res

class Assign:
	def __init__(self, ident, expr):
		self.ident = ident
		self.expr = expr
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return f"{self.ident!r} = {self.expr!r}"

class Call:
	def __init__(self, ident, args):
		self.ident = ident
		self.args = args
		self.data_type = ident.data_type.return_type
		self.is_const = False
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return repr(self.ident) + "()"

class Print:
	def __init__(self, expr_list):
		self.expr_list = expr_list
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return "print " + repr(self.expr_list)

class Return:
	def __init__(self, expr = None):
		self.expr = expr
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return "return " + (repr(self.expr) if self.expr else "")

class IfStmt:
	def __init__(self, cond, body, else_body = None):
		self.cond = cond
		self.body = body
		self.else_body = else_body
	
	def __repr__(self):
		return self.to_str()
	
	def to_str(self, level = 0):
		return (
			"if "
			+ repr(self.cond)
			+ " {\n"
			+ self.body.to_str(level + 1)
			+ " " * level + "}"
			+ (
				" else {\n"
				+ self.else_body.to_str(level + 1)
				+ " " * level + "}"
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
			"while "
			+ repr(self.cond)
			+ " {\n"
			+ self.body.to_str(level + 1)
			+ " " * level + "}"
		)

class ArrayType:
	def __init__(self, base_type):
		self.base_type = base_type
	
	def __repr__(self):
		return "[..]" + repr(self.base_type)
	
	def __eq__(self, other):
		return type(other) is ArrayType and self.base_type == other.base_type

class UnknownType:
	def __repr__(self):
		return "<unknown-type>"
	
	def __eq__(self, other):
		return type(self) is type(other)
	
	def __bool__(self):
		return False

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

class ChainOp:
	def __init__(self, start, operands, ops, data_type):
		self.start = start
		self.operands = operands
		self.ops = ops
		self.data_type = data_type
		self.is_const = self.all_const()
	
	def all_const(self):
		for operand in self.operands:
			if not operand.is_const:
				return False
		
		return self.start.is_const
	
	def __repr__(self):
		return "(" + repr(self.start) + "".join(
			" " + op.value + " " + repr(operand)
			for op, operand in zip(self.ops, self.operands)
		) + ")"

class Cast:
	def __init__(self, expr, target_type, is_const = None):
		self.expr = expr
		self.target_type = target_type
		self.is_const = is_const if is_const is not None else expr.is_const
		self.data_type = self.target_type
	
	def __repr__(self):
		return f"cast({self.target_type} : {self.expr})"

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
FloatType = PrimType("float")

class VoidType:
	def __repr__(self):
		return "void"
	
	def __eq__(self, other):
		return type(self) is type(other)
	
	def __bool__(self):
		return False

VoidType = VoidType()

class FuncType:
	def __init__(self, return_type = None):
		self.return_type = return_type
	
	def __repr__(self):
		return "func()" + (" : " + repr(self.return_type))
	
	def __eq__(self, other):
		return type(self) is type(other)

