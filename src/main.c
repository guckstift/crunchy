#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crunchy.h"

void error(char *msg)
{
	printf("error: %s\n", msg);
	exit(EXIT_FAILURE);
}

char *find_src_start(Token *token)
{
	Token *bof = token;
	while(bof->kind != TK_BOF) bof --;
	return bof->start;
}

char *find_line_start(Token *token, char *src_start)
{
	char *line_start = token->start;
	while(line_start > src_start && line_start[-1] != '\n') line_start --;
	return line_start;
}

void error_at(Token *at, char *msg)
{
	char *src_file_start = find_src_start(at);
	char *line_start = find_line_start(at, src_file_start);
	print("%[f00]error:%[] %s\n", msg);
	print("%[888]");
	int64_t offset = print("%i", at->line);
	offset += print(":%[] ");

	for(char *p = line_start; *p && *p != '\n'; p++) {
		if(*p == '\t') {
			if(p < at->start)
				offset += fprintf(stdout, "  ");
			else
				fprintf(stdout, "  ");
		}
		else {
			fputc(*p, stdout);
			if(p < at->start) offset ++;
		}
	}

	printf("\n");
	for(int64_t i=0; i < offset; i++) printf(" ");
	print("%[f00]^%[]\n");
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
	printf("\n# SOURCE\n");
	printf("%s\n", src);

	Token *tokens = 0;
	int64_t token_count = lex(src, &tokens);
	printf("\n# TOKENS\n");
	print_token_list(tokens);

	Block *block = parse(tokens);
	printf("\n# AST\n");
	print_block(block);

	analyse(block);
	printf("\n# AST (analysed)\n");
	print_block(block);

	int64_t input_filename_length = strlen(input_file);
	char *output_file = malloc(input_filename_length + 2 + 1);
	memcpy(output_file, input_file, input_filename_length);
	memcpy(output_file + input_filename_length, ".c", 2);
	output_file[input_filename_length + 2] = 0;

	generate(block, output_file);

	return 0;
}