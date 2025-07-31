#include <stdlib.h>
#include <string.h>
#include "crunchy.h"

int main(int argc, char **argv)
{
	if(argc < 2) {
		error("missing input file parameter");
	}

	char *input_file = argv[1];
	char *src = load_text_file(input_file);
	print("\n%[ff0]# SOURCE%[]\n");
	print("%s\n", src);

	Token *tokens = 0;
	int64_t token_count = lex(src, &tokens);
	print("\n%[ff0]# TOKENS%[]\n");
	print_token_list(tokens);

	Block *block = parse(tokens);
	print("\n%[ff0]# AST%[]\n");
	print_block(block);

	analyse(block);
	print("\n%[ff0]# AST (analysed)%[]\n");
	print_block(block);

	int64_t input_filename_length = strlen(input_file);
	char *output_file = malloc(input_filename_length + 2 + 1);
	memcpy(output_file, input_file, input_filename_length);
	memcpy(output_file + input_filename_length, ".c", 2);
	output_file[input_filename_length + 2] = 0;

	generate(block, output_file);

	return 0;
}