char *kw_break = 0;
char *kw_continue = 0;
char *kw_else = 0;
char *kw_export = 0;
char *kw_float = 0;
char *kw_func = 0;
char *kw_if = 0;
char *kw_int = 0;
char *kw_import = 0;
char *kw_length = 0;
char *kw_print = 0;
char *kw_return = 0;
char *kw_struct = 0;
char *kw_while = 0;

typedef enum {
	PN_AND, PN_OR,
	PN_EQU, PN_NEQU, PN_LTEQU, PN_GTEQU,
	PN_DECL,
	PN_ADDASSIGN, PN_SUBASSIGN, PN_MULASSIGN, PN_DIVASSIGN, PN_MODASSIGN,
	PN_PLUS, PN_MINUS, PN_MUL, PN_DIV, PN_MOD,
	PN_GT, PN_LT, PN_ADDR,
	PN_COLON, PN_ASSIGN, PN_COMMA, PN_PERIOD,
	PN_LPAREN, PN_RPAREN, PN_LCURLY, PN_RCURLY, PN_LBRACK, PN_RBRACK,
} Punct;

char *puncts[] = {
	"&&", "||",
	"==", "!=", "<=", ">=",
	":=",
	"+=", "-=", "*=", "/=", "%=",
	"+", "-", "*", "/", "%",
	">", "<", "@",
	":", "=", ",", ".",
	"(", ")", "{", "}", "[", "]",
	0
};

char *pool_insert(char *str)
{
	size_t len = strlen(str);
	pool_count ++;
	str_pool = realloc(str_pool, sizeof(String) * pool_count);
	String *pooled = &str_pool[pool_count - 1];
	pooled->len = len;
	pooled->str = str;
	return pooled->str;
}

char *pool_str(char *str, size_t len)
{
	if(len == 0)
		len = strlen(str);
	
	for(size_t i=0; i<pool_count; i++) {
		String *pooled = &str_pool[i];
		
		if(pooled->len == len && strncmp(pooled->str, str, len) == 0)
			return pooled->str;
	}
	
	pool_count ++;
	str_pool = realloc(str_pool, sizeof(String) * pool_count);
	String *pooled = &str_pool[pool_count - 1];
	pooled->len = len;
	pooled->str = malloc(len + 1);
	pooled->str[len] = 0;
	memcpy(pooled->str, str, len);
	return pooled->str;
}

char *clone_strpart(char *start, size_t length)
{
	char *cloned = malloc(length + 1);
	memcpy(cloned, start, length);
	cloned[length] = 0;
	return cloned;
}

size_t match_strpart(char *start, char *needle)
{
	size_t i = 0;
	
	while(start[i] && needle[i] && start[i] == needle[i])
		i ++;
	
	if(needle[i] == 0)
		return i;
	
	return 0;
}

size_t match_punct(Punct *punct_id)
{
	for(int i=0; puncts[i]; i++) {
		char *punct = puncts[i];
		size_t length = match_strpart(src, punct);
		
		if(length) {
			*punct_id = i;
			return length;
		}
	}
	
	return 0;
}

void emit_token(Kind kind)
{
	token = unit->tokens + unit->tcount;
	token->kind = kind;
	token->line = line;
	token->pos = pos;
	token->text = start;
	token->length = src - start;
	unit->tcount ++;
}

void lex_unit()
{
	if(str_pool == 0) {
		kw_break = pool_insert("break");
		kw_continue = pool_insert("continue");
		kw_else = pool_insert("else");
		kw_export = pool_insert("export");
		kw_float = pool_insert("float");
		kw_func = pool_insert("func");
		kw_if = pool_insert("if");
		kw_int = pool_insert("int");
		kw_import = pool_insert("import");
		kw_length = pool_insert("length");
		kw_print = pool_insert("print");
		kw_return = pool_insert("return");
		kw_struct = pool_insert("struct");
		kw_while = pool_insert("while");
	}
	
	filename = unit->filename;
	line = 1;
	pos = 1;
	src = unit->source;
	unit->tokens = malloc(sizeof(Token) * (unit->length + 1));
	
	while(*src) {
		start = src;
		
		if(*src == '\n') {
			src ++;
			line ++;
			pos = 1;
		}
		else if(*src == '#') {
			size_t len = strcspn(src, "\n");
			src += len;
			pos += len;
		}
		else if(isspace(*src)) {
			src ++;
			pos ++;
		}
		else if(*src == '/' && src[1] == '*') {
			src += 2;
			
			while(*src && !(*src == '*' && src[1] == '/')) {
				if(*src == '\n') {
					line ++;
					pos = 1;
				}
				else
					pos ++;
				
				src ++;
			}
			
			if(*src == 0)
				error("multi-line comments must end with */");
			
			src += 2;
		}
		else if(isdigit(*src)) {
			size_t val = 0;
			
			while(isdigit(*src)) {
				val *= 10;
				val += *src - '0';
				src ++;
			}
			
			if(*src == '.') {
				src ++;
				
				if(!isdigit(*src))
					error("'.' must be followed by a digit");
				
				while(isdigit(*src)) {
					src ++;
				}
				
				size_t len = src - start;
				char buf[len + 1];
				memcpy(buf, start, len);
				buf[len] = 0;
				double fval = strtod(buf, 0);
				emit_token(FLOAT);
				token->fval = fval;
			}
			else {
				emit_token(INTEGER);
				token->val = val;
			}
			
			pos += src - start;
		}
		else if(isalpha(*src) || *src == '_') {
			while(isalnum(*src))
				src ++;
			
			emit_token(IDENT);
			pos += src - start;
		}
		else if(*src == '"') {
			src ++;
			start = src;
			
			while(*src && *src != '"' && *src >= 0x20 && *src <= 0x7f)
				src ++;
			
			if(*src != '"')
				error("string literals must end with \"");
			
			emit_token(STRING);
			src ++;
			pos += src - start;
		}
		else {
			Punct punct;
			size_t punctlen = match_punct(&punct);
			
			if(punctlen) {
				src += punctlen;
				emit_token(PUNCT);
				token->punct = punct;
				token->text = puncts[token->punct];
				pos += src - start;
			}
			else
				error("unrecognized token 0x%x", *src);
		}
	}
	
	emit_token(END);
	unit->tokens = realloc(unit->tokens, sizeof(Token) * unit->tcount);
	
	for(Token *token = unit->tokens; token->kind != END; token ++) {
		if(token->kind == IDENT)
			token->text = pool_str(token->text, token->length);
		else if(token->kind == STRING) {
			size_t real_strlen = 0;
			char *decoded_str = malloc(token->length + 1);
			char *coded_str = token->text;
			
			while(coded_str < token->text + token->length) {
				if(*coded_str == '\\') {
					coded_str ++;
					
					if(*coded_str == 'n') {
						decoded_str[real_strlen++] = '\n';
					}
				}
				else {
					decoded_str[real_strlen++] = *coded_str;
				}
				
				coded_str ++;
			}
			
			token->tval = decoded_str;
			token->tval_len = real_strlen;
			decoded_str[real_strlen++] = 0;
		}
	}
}
