#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "crunchy.h"

int64_t lex(char *src, Token **tokens_out)
{
	Token *tokens = 0;
	int64_t count = 0;

	while(*src) {
		TokenKind kind = TK_INVALID;
		char *start = src;
		int64_t ival = 0;

		if(isspace(*src)) {
			src ++;
			continue;
		}
		else if(isdigit(*src)) {
			while(isdigit(*src)) {
				ival = ival * 10 + (*src - '0');
				src ++;
			}

			kind = TK_INT;
		}
		else if(isalpha(*src)) {
			while(isalpha(*src) || isdigit(*src)) {
				src ++;
			}

			int64_t length = src - start;
			kind = TK_IDENT;

			#define _(a) if(length == sizeof(#a)-1 && memcmp(start, #a, sizeof(#a)-1) == 0) { kind = KW_ ## a ; } else
			KEYWORDS;
			#undef _
		}

		#define _(a, b) else if(*src == a) { kind = PT_ ## b ; src ++; }
		PUNCTS
		#undef _

		else {
			printf("(%i) %c\n", *src, *src);
			error("unrecognized token");
		}

		count ++;
		tokens = realloc(tokens, sizeof(Token) * count);
		tokens[count - 1] = (Token){.kind = kind, .start = start, .end = src, .ival = ival};
	}

	count ++;
	tokens = realloc(tokens, sizeof(Token) * count);
	tokens[count - 1] = (Token){.kind = TK_EOF, .start = src, .end = src};

	*tokens_out = tokens;
	return count;
}