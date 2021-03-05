#include "crunchy.h"

Unit *build_unit(char *filename)
{
	Unit *unit = malloc(sizeof(Unit));
	unit->filename = filename;
	unit->source = read_file(unit->filename);
	unit->tokens = lex(unit->source);
	dump_tokens(unit->tokens);
	unit->ast = parse(unit->tokens);
	dump_ast(unit->ast);
	resolve(unit->ast);
	dump_ast(unit->ast);
	generate(unit->ast);
	return unit;
}

int main(int argc, char *argv[])
{
	if(argc < 2) {
		return 0;
	}
	
	char *main_filename = argv[1];
	Unit *main_unit = build_unit(main_filename);
	//run(main_unit);
	return 0;
}
