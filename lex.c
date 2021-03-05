#include "crunchy.h"

static Tokens *tokens;
static size_t line;

static Token *create_token(TokenKind kind, char *name, char *start, char *end)
{
	size_t length = end - start;
	Token *token = malloc(sizeof(Token));
	token->kind = kind;
	token->name = name;
	token->line = line;
	token->length = length;
	token->text = clone_substring(start, length);
	token->next = 0;
	return token;
}

static void emit_token(TokenKind kind, char *name, char *start, char *end)
{
	Token *token = create_token(kind, name, start, end);
	
	if(tokens->first) {
		tokens->last->next = token;
	}
	else {
		tokens->first = token;
	}
	
	tokens->last = token;
}

static size_t match_punct(char *src)
{
	static char *puncts[] = {
		":=",
		"+", "-", "*", "/",
		"=", ":",
		"(", ")", "{", "}",
		0
	};
	
	for(int i=0; puncts[i]; i++) {
		char *punct = puncts[i];
		size_t length = match_substring(src, punct);
		
		if(length) {
			return length;
		}
	}
	
	return 0;
}

Tokens *lex(char *src)
{
	tokens = calloc(1, sizeof(Tokens));
	line = 0;
	
	while(*src) {
		if(*src == '\n') {
			src ++;
			line ++;
			continue;
		}
		
		if(*src == '#') {
			src += strcspn(src, "\n");
			continue;
		}
		
		char *start = src;
		
		while(isdigit(*src)) {
			src ++;
		}
		
		if(src > start) {
			emit_token(TK_INT, "INT", start, src);
			continue;
		}
		
		while(isalnum(*src) || *src == '_') {
			src ++;
		}
		
		if(src > start) {
			emit_token(TK_IDENT, "IDENT", start, src);
			continue;
		}
		
		int length = match_punct(src);
		
		if(length) {
			src += length;
			emit_token(TK_PUNCT, "PUNCT", start, src);
			continue;
		}
		
		src ++;
	}
	
	emit_token(TK_END, "END", src, src);
	
	return tokens;
}
