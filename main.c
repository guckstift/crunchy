#include "common.c"
#include "data.c"
#include "build.c"
#include "lex.c"
#include "parse.c"
#include "analyze.c"
#include "generate.c"
#include "dump.c"

void run_project()
{
	system(project->exepath);
}

int main(int argc, char *argv[])
{
	if(argc < 2)
		error("no input file given");
	
	char *main_filepath = argv[1];
	build_project(main_filepath);
	run_project();
	return 0;
}
