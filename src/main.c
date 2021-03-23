#include "data.c"
#include "common.c"
#include "d2s.c"
#include "build.c"
#include "lex.c"
#include "dump.c"
#include "parse.c"
#include "analyze.c"
#include "generate.c"

int main(int argc, char *argv[])
{
	if(argc < 2)
		error("no input file given");
	
	char *main_filepath = argv[1];
	build_project(main_filepath);
	system(project->exepath);
	return 0;
}
