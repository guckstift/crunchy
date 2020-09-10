import pprint
import ast

operator_tiers = [
	[False, ["&&", "||"]],
	[False, ["==", "!=", "<=", ">=", "<", ">"]],
	[True,  ["+", "-"]],
	[False, ["*", "/"]],
]

prim_types = [ast.BoolType, ast.IntType, ast.FloatType, ast.StringType]
num_types = [ast.BoolType, ast.IntType, ast.FloatType]
int_types = [ast.BoolType, ast.IntType]

binop_type_inferences = {
	"+": [
		[ast.StringType, prim_types,     ast.StringType],
		[prim_types,     ast.StringType, ast.StringType],
	],
	("+", "-", "*"): [
		[ast.FloatType, num_types,     ast.FloatType],
		[num_types,     ast.FloatType, ast.FloatType],
		[int_types,     int_types,     ast.IntType],
	],
	"/": [
		[num_types, num_types,     ast.FloatType],
	],
	("==", "!=", "<=", ">=", "<", ">"): [
		[num_types, num_types, ast.BoolType],
	],
	("==", "!="): [
		[ast.StringType, ast.StringType, ast.BoolType],
	],
	("&&", "||"): [
		[ast.BoolType, ast.BoolType, ast.BoolType],
	],
}

binop_cast_table = {
	("==", "!=", "<=", ">=", "<", ">", "+", "-", "*"): [
		[ast.FloatType,  int_types,      None,           ast.FloatType],
		[int_types,      ast.FloatType,  ast.FloatType,  None],
		[ast.IntType,    ast.BoolType,   None,           ast.IntType],
		[ast.BoolType,   ast.IntType,    ast.IntType,    None],
		[ast.StringType, num_types,      None,           ast.StringType],
		[num_types,      ast.StringType, ast.StringType, None],
	],
	"/": [
		[ast.FloatType,  int_types,      None,           ast.FloatType],
		[int_types,      ast.FloatType,  ast.FloatType,  None],
		[int_types,      int_types,      ast.FloatType,  ast.FloatType],
	]
}

for key in binop_type_inferences.copy():
	if type(key) is tuple:
		src_table = binop_type_inferences[key]
		
		for part in key:
			if part not in binop_type_inferences:
				binop_type_inferences[part] = []
			
			dest_table = binop_type_inferences[part]
			dest_table.extend(src_table)
		
		del binop_type_inferences[key]

for key in binop_cast_table.copy():
	if type(key) is tuple:
		src_table = binop_cast_table[key]
		
		for part in key:
			if part not in binop_cast_table:
				binop_cast_table[part] = []
			
			dest_table = binop_cast_table[part]
			dest_table.extend(src_table)
		
		del binop_cast_table[key]

#pprint.pprint(binop_cast_table)

def infer_binop_type(left_type, right_type, op):
	table = binop_type_inferences[op]
	
	for entry in table:
		left, right, result = entry
		
		if (
			(left_type == left or type(left) is list and left_type in left) and
			(right_type == right or type(right) is list and right_type in right)
		):
			return result
	
	return ast.UnknownType

def unify_binop(left, right, op):
	if op not in binop_cast_table:
		return None, None
	
	table = binop_cast_table[op]
	left_type = left.data_type
	right_type = right.data_type
	
	for entry in table:
		left_t, right_t, left_cast, right_cast = entry
		
		if (
			(left_type == left_t or type(left_t) is list and left_type in left_t) and
			(right_type == right_t or type(right_t) is list and right_type in right_t)
		):
			if left_cast:
				left = ast.Cast(left, left_cast)
			
			if right_cast:
				right= ast.Cast(right, right_cast)
			
			return left, right
	
	return None, None




