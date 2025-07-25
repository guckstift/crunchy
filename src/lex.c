#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "crunchy.h"

int64_t lex(char *src, Token **tokens_out)
{
	Token *tokens = 0;
	int64_t count = 0;
	int64_t line = 1;
	char *start = src;

	#define emit_token(k, ...) { \
		count ++; \
		tokens = realloc(tokens, sizeof(Token) * count); \
		tokens[count - 1] = (Token){.kind = k, .start = start, .length = src - start, .line = line, __VA_ARGS__}; \
	}

	emit_token(TK_BOF);

	while(*src) {
		start = src;
		int64_t ival = 0;

		if(isspace(*src)) {
			if(*src == '\n') line ++;
			src ++;
		}
		else if(*src == '#') {
			while(*src && *src != '\n') src ++;
		}
		else if(isdigit(*src)) {
			while(isdigit(*src)) {
				ival = ival * 10 + (*src - '0');
				src ++;
			}

			emit_token(TK_INT, .ival = ival);
		}
		else if(isalpha(*src)) {
			while(isalpha(*src) || isdigit(*src)) src ++;
			int64_t length = src - start;
			Kind kind = TK_IDENT;

			#define _(a) if(length == sizeof(#a)-1 && memcmp(start, #a, sizeof(#a)-1) == 0) { kind = KW_ ## a ; } else
			KEYWORDS;
			#undef _

			emit_token(kind);
		}
		else if(*src == '"') {
			int64_t str_length = 0;
			src ++;

			while(isprint(*src) && *src != '"') {
				if(*src == '\\') {
					src ++;
					if(*src == '"') src ++;
					else error("invalid escape character");
					str_length ++;
				}
				else {
					src ++;
					str_length ++;
				}
			}

			if(*src != '"') error("unterminated string");
			src ++;
			emit_token(TK_STRING, .str_length = str_length);
		}

		#define _(a, b) else if(*src == a) { src ++; emit_token(PT_ ## b); }
		PUNCTS
		#undef _

		else {
			printf("(%i) %c\n", *src, *src);
			error("unrecognized token");
		}
	}

	start = src;
	emit_token(TK_EOF);

	for(Token *token = tokens; token->kind != TK_EOF; token ++) {
		if(token->kind == TK_STRING) {
			token->chars = calloc(token->str_length, 1);
			char *output = token->chars;

			for(char *input = token->start + 1; *input != '"'; input ++) {
				if(*input == '\\') input ++;
				*output = *input;
				output ++;
			}
		}
	}

	*tokens_out = tokens;
	return count;
}