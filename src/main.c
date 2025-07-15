#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crunchy.h"

void error(char *msg)
{
	printf("error: %s\n", msg);
	exit(EXIT_FAILURE);
}

char *load_text_file(char *file_name)
{
	FILE *fs = fopen(file_name, "rb");

	if(!fs) {
		error("could not open input file");
	}

	fseek(fs, 0, SEEK_END);
	long size = ftell(fs);
	rewind(fs);
	char *text = malloc(size + 1);
	text[size] = 0;
	fread(text, 1, size, fs);
	fclose(fs);
	return text;
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		error("missing input file parameter");
	}

	char *input_file = argv[1];
	char *src = load_text_file(input_file);

	printf("content: %s \n", src);

	Token *tokens = 0;
	int64_t token_count = lex(src, &tokens);

	for(Token *token = tokens; token->kind != TK_EOF; token ++) {
		printf("%s ",
			token->kind == TK_INT       ? "<INT>       " :
			token->kind == TK_IDENT     ? "<IDENT>     " :
			token->kind == KW_var       ? "<KEYWORD>   " :
			token->kind == PT_EQUALS    ? "<EQUALS>    " :
			token->kind == PT_SEMICOLON ? "<SEMICOLON> " :
			token->kind == PT_COLON     ? "<COLON>     " :
			"<invalid-token>"
		);

		fwrite(token->start, 1, token->end - token->start, stdout);
		printf("\n");
	}

	Stmt *stmts = parse(tokens);
	print(stmts);
	analyse(stmts);
	print(stmts);

	int64_t input_filename_length = strlen(input_file);
	char *output_file = malloc(input_filename_length + 2 + 1);
	memcpy(output_file, input_file, input_filename_length);
	memcpy(output_file + input_filename_length, ".c", 2);
	output_file[input_filename_length + 2] = 0;

	generate(stmts, output_file);

	return 0;
}