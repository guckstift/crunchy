#include "data.c"
#include "common.c"
#include "d2s.c"
#include "dump.c"
#include "build.c"
#include "lex.c"
#include "parse.c"
#include "analyze.c"
#include "generate.c"

int main(int argc, char *argv[])
{
	kw_break = pool_str("break", 0);
	kw_continue = pool_str("continue", 0);
	kw_else = pool_str("else", 0);
	kw_export = pool_str("export", 0);
	kw_float = pool_str("float", 0);
	kw_func = pool_str("func", 0);
	kw_if = pool_str("if", 0);
	kw_int = pool_str("int", 0);
	kw_import = pool_str("import", 0);
	kw_print = pool_str("print", 0);
	kw_return = pool_str("return", 0);
	kw_struct = pool_str("struct", 0);
	kw_while = pool_str("while", 0);
	
	if(argc < 2)
		error("no input file given");
	
	char *main_filepath = argv[1];
	build_project(main_filepath);
	system(project->exepath);
	return 0;
}
