char *puncts[] = {
	"&&", "||",
	"==", "!=", "<=", ">=",
	":=",
	"+", "-", "*", "/", "%",
	">", "<",
	":", "=", ",",
	"(", ")", "{", "}", "[", "]",
	0
};

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

size_t match_punct()
{
	for(int i=0; puncts[i]; i++) {
		char *punct = puncts[i];
		size_t length = match_strpart(src, punct);
		
		if(length)
			return length;
	}
	
	return 0;
}

void emit_token(Kind kind)
{
	token = create(Token);
	token->kind = kind;
	token->line = line;
	token->pos = pos;
	
	if(tokens->count) {
		tokens->last->next = token;
		tokens->last = token;
	}
	else {
		tokens->first = token;
		tokens->last = token;
	}
	
	tokens->count ++;
}

void lex_unit()
{
	unit->tokens = create(TokenList);
	filename = unit->filename;
	line = 1;
	pos = 1;
	src = unit->source;
	tokens = unit->tokens;
	
	while(*src) {
		char *start = src;
		
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
				
				char *fval_text = clone_strpart(start, src - start);
				double fval = strtod(fval_text, 0);
				free(fval_text);
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
			token->text = clone_strpart(start, src - start);
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
			token->text = clone_strpart(start, src - start);
			src ++;
			pos += src - start;
		}
		else {
			size_t punctlen = match_punct(src);
			
			if(punctlen) {
				emit_token(PUNCT);
				token->text = clone_strpart(src, punctlen);
				src += punctlen;
				pos += src - start;
			}
			else
				error("unrecognized token 0x%x", *src);
		}
	}
	
	emit_token(END);
}
