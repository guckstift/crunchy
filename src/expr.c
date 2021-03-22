char *optable[] = {
	"|| ",
	"&& ",
	"== != <= >= < > ",
	"+ - ",
	"* / % ",
	0,
};

Expr *parse_prim()
{
	if(is_kind(INTEGER) || is_kind(FLOAT) || is_kind(IDENT)) {
		Expr *prim = create_expr(PRIM);
		prim->prim = next_token();
		prim->isconst = prim->prim->kind != IDENT;
		prim->islvalue = prim->prim->kind == IDENT;
		return prim;
	}
	
	return 0;
}

Expr *parse_ident_prim()
{
	if(is_kind(IDENT)) {
		Expr *prim = create_expr(PRIM);
		prim->prim = next_token();
		prim->islvalue = 1;
		return prim;
	}
	
	return 0;
}

Expr *parse_literal_prim()
{
	if(is_kind(INTEGER) || is_kind(FLOAT)) {
		Expr *prim = create_expr(PRIM);
		prim->prim = next_token();
		prim->isconst = 1;
		return prim;
	}
	
	return 0;
}

Expr *parse_array()
{
	if(parse_punct(PN_LBRACK) == 0)
		return 0;
	
	Expr *first = 0;
	Expr *last = 0;
	size_t length = 0;
	int isconst = 1;
	
	while(1) {
		Expr *item = parse_expr();
		
		if(item == 0)
			break;
		
		if(first) {
			last->next = item;
			last = item;
		}
		else {
			first = item;
			last = item;
		}
		
		length ++;
		isconst = isconst && item->isconst;
		
		if(parse_punct(PN_COMMA) == 0)
			break;
	}
	
	if(length == 0)
		error("empty array literals are not allowed");
	
	if(parse_punct(PN_RBRACK) == 0)
		error("array literal must be terminated with ']'");
	
	if(isconst == 0)
		error("array literals must be constant");
	
	Expr *array = create_expr(ARRAY);
	array->child = first;
	array->length = length;
	array->isconst = isconst;
	return array;
}

Expr *parse_call()
{
	Token *start = token;
	Token *ident = parse_kind(IDENT);
	
	if(ident == 0)
		return 0;
	
	if(parse_punct(PN_LPAREN) == 0) {
		seek_token(start);
		return 0;
	}
	
	Expr *first = 0;
	Expr *last = 0;
	size_t arg_count = 0;
	
	while(1) {
		Expr *arg = parse_expr();
		
		if(arg == 0)
			break;
		
		if(first) {
			last->next = arg;
			last = arg;
		}
		else {
			first = arg;
			last = arg;
		}
		
		arg_count ++;
		
		if(parse_punct(PN_COMMA) == 0)
			break;
	}
	
	if(parse_punct(PN_RPAREN) == 0)
		error("expected ')' after argument list");
	
	Expr *call = create_expr(CALL);
	call->ident = ident;
	call->child = first;
	call->length = arg_count;
	return call;
}

Expr *parse_target()
{
	Expr *expr = parse_ident_prim();
	
	if(expr == 0)
		return 0;
	
	while(1) {
		if(parse_punct(PN_LBRACK)) {
			Expr *index = parse_expr();
			
			if(index == 0)
				error("expected index of subscript");
			
			if(parse_punct(PN_RBRACK) == 0)
				error("expected ']' after index");
			
			Expr *subscript = create_expr(SUBSCRIPT);
			subscript->left = expr;
			subscript->right = index;
			subscript->islvalue = expr->islvalue;
			expr = subscript;
		}
		else if(parse_punct(PN_PERIOD)) {
			Expr *ident = parse_ident_prim();
			
			if(ident == 0)
				error("expected member identifier after '.'");
			
			Expr *member = create_expr(MEMBER);
			member->left = expr;
			member->right = ident;
			member->islvalue = 1;
			expr = member;
		}
		else
			break;
	}
	
	return expr;
}

Expr *parse_postfix()
{
	Expr *expr = parse_call();
	
	if(expr == 0)
		expr = parse_ident_prim();
	
	if(expr == 0)
		return 0;
	
	while(1) {
		if(parse_punct(PN_LBRACK)) {
			Expr *index = parse_expr();
			
			if(index == 0)
				error("expected index of subscript");
			
			if(parse_punct(PN_RBRACK) == 0)
				error("expected ']' after index");
			
			Expr *subscript = create_expr(SUBSCRIPT);
			subscript->left = expr;
			subscript->right = index;
			subscript->islvalue = expr->islvalue;
			expr = subscript;
		}
		else if(parse_punct(PN_PERIOD)) {
			Expr *ident = parse_ident_prim();
			
			if(ident == 0)
				error("expected member identifier after '.'");
			
			Expr *member = create_expr(MEMBER);
			member->left = expr;
			member->right = ident;
			member->islvalue = 1;
			expr = member;
		}
		else
			break;
	}
	
	return expr;
}

Expr *parse_pointer()
{
	if(parse_punct(PN_LT)) {
		Expr *ptr = parse_pointer();
		
		if(ptr == 0)
			error("expected pointer to dereference");
			
		Expr *deref = create_expr(DEREF);
		deref->child = ptr;
		return deref;
	}
	
	if(parse_punct(PN_ADDR)) {
		Expr *ptr = parse_pointer();
		
		if(ptr == 0)
			error("expected pointer to take the address from");
			
		Expr *address = create_expr(ADDRESS);
		address->child = ptr;
		return address;
	}
	
	if(parse_punct(PN_GT)) {
		Expr *target = parse_target();
		
		if(target == 0)
			error("expected target to point to");
		
		Expr *ptr = create_expr(PTR);
		ptr->child = target;
		return ptr;
	}
	
	return parse_postfix();
}

Expr *parse_prefix()
{
	Token *op = 0;
	
	(op = parse_punct(PN_PLUS)) ||
	(op = parse_punct(PN_MINUS)) ||
	0 ;
	
	if(op) {
		Expr *child = parse_prefix();
		
		if(child == 0)
			error("expected expression after unary operator '%s'", op->text);
		
		Expr *unary = create_expr(UNARY);
		unary->child = child;
		unary->op = op;
		return unary;
	}
	
	Expr *expr = parse_array();
	
	if(expr)
		return expr;
	
	expr = parse_literal_prim();
	
	if(expr)
		return expr;
	
	return parse_pointer();
}

Expr *parse_chain(int tier)
{
	char *ops = optable[tier];
	
	if(ops == 0)
		return parse_prefix();
	
	Expr *left = parse_chain(tier + 1);
	
	if(left == 0)
		return 0;
	
	while(1) {
		Token *op = parse_op(ops);
		
		if(op == 0)
			return left;
		
		Expr *right = parse_chain(tier + 1);
		
		if(right == 0)
			error("right side expected after binary operator '%s'", op->text);
		
		Expr *chain = create_expr(CHAIN);
		chain->left = left;
		chain->right = right;
		chain->op = op;
		chain->tier = tier;
		chain->isconst = left->isconst && right->isconst;
		left = chain;
	}
}

Expr *parse_expr()
{
	return parse_chain(0);
}
