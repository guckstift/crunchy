Project *project = 0;
Unit *unit = 0;

void lex_unit();
void parse_unit();
void analyze_unit();
void generate_code();
void dump_tokens();
void dump_ast();

char *get_abspath(char *filepath)
{
	char *abspath = realpath(filepath, 0);
	
	if(abspath == 0) {
		error("could not resolve path '%s'", filepath);
	}
	
	return abspath;
}

char *get_filename(char *filepath)
{
	return basename(filepath);
}

void add_unit()
{
	if(project->units->count) {
		project->units->last->next = unit;
		project->units->last = unit;
	}
	else {
		project->units->first = unit;
		project->units->last = unit;
		project->main = unit;
	}
	
	project->units->count ++;
}

void load_unit()
{
	FILE *fs = fopen(unit->abspath, "rb");
	
	if(fs == 0) {
		error("could not open file '%s'", unit->abspath);
	}
	
	fseek(fs, 0, SEEK_END);
	unit->length = ftell(fs);
	fseek(fs, 0, SEEK_SET);
	unit->source = malloc(unit->length + 1);
	fread(unit->source, 1, unit->length, fs);
	unit->source[unit->length] = 0;
	fclose(fs);
}

void build_unit(char *filepath)
{
	unit = create(Unit);
	unit->filepath = filepath;
	unit->abspath = get_abspath(filepath);
	unit->filename = get_filename(filepath);
	add_unit();
	load_unit();
	lex_unit();
	dump_tokens();
	parse_unit();
	dump_ast();
	analyze_unit();
	dump_ast();
	printf("\nc code for unit\n\n");
	generate_code();
}

void build_project(char *main_filepath)
{
	project = create(Project);
	project->units = create(UnitList);
	build_unit(main_filepath);
}
