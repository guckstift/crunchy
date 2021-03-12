Project *project = 0;
Unit *unit = 0;

void lex_unit();
void parse_unit();
void analyze_unit();
void generate_code();
void dump_tokens();
void dump_ast();

void mkdir_p(char *path)
{
	struct stat stat_buf;
	
	for(char *x = path; *x; x ++) {
		if(*x == '/') {
			*x = 0;
			
			if(stat(path, &stat_buf) != 0) {
				mkdir(path, 0755);
			}
			
			*x = '/';
		}
	}
}

char *get_abspath(char *filepath)
{
	char *abspath = realpath(filepath, 0);
	
	if(abspath == 0)
		error("could not resolve path '%s'", filepath);
	
	return abspath;
}

char *get_filename(char *filepath)
{
	return basename(filepath);
}

char *append_str(char *str, char *extra)
{
	size_t str_len = str ? strlen(str) : 0;
	size_t extra_len = strlen(extra);
	size_t len = str_len + extra_len;
	str = realloc(str, len + 1);
	memcpy(str + str_len, extra, extra_len + 1);
	return str;
}

char *get_cache_path(char *abspath, char *ext)
{
	char *cpath = 0;
	cpath = append_str(cpath, "/home/danny/.crunchy");
	cpath = append_str(cpath, abspath);
	cpath = append_str(cpath, ext);
	return cpath;
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
	
	if(fs == 0)
		error("could not open file '%s'", unit->abspath);
	
	fseek(fs, 0, SEEK_END);
	unit->length = ftell(fs);
	fseek(fs, 0, SEEK_SET);
	unit->source = malloc(unit->length + 1);
	fread(unit->source, 1, unit->length, fs);
	unit->source[unit->length] = 0;
	fclose(fs);
}

void compile_unit()
{
	char *cc_cmd = 0;
	cc_cmd = append_str(cc_cmd, "gcc -c -ansi -pedantic-errors -o ");
	cc_cmd = append_str(cc_cmd, unit->opath);
	cc_cmd = append_str(cc_cmd, " ");
	cc_cmd = append_str(cc_cmd, unit->cpath);
	
	if(system(cc_cmd) != 0)
		error("compilation of c code failed");
}

void build_unit(char *filepath)
{
	unit = create(Unit);
	unit->filepath = filepath;
	unit->abspath = get_abspath(filepath);
	unit->filename = get_filename(filepath);
	unit->cpath = get_cache_path(unit->abspath, ".c");
	unit->opath = get_cache_path(unit->abspath, ".o");
	add_unit();
	load_unit();
	lex_unit();
	dump_tokens();
	parse_unit();
	dump_ast();
	analyze_unit();
	dump_ast();
	mkdir_p(unit->cpath);
	generate_code();
	compile_unit();
}

void link_project()
{
	project->exepath = get_cache_path(project->main->abspath, ".exe");
	char *cc_cmd = 0;
	cc_cmd = append_str(cc_cmd, "gcc -o ");
	cc_cmd = append_str(cc_cmd, project->exepath);
	cc_cmd = append_str(cc_cmd, " ");
	cc_cmd = append_str(cc_cmd, project->main->opath);
	system(cc_cmd);
	
	if(system(cc_cmd) != 0)
		error("linking of objects failed");
}

void build_project(char *main_filepath)
{
	project = create(Project);
	project->units = create(UnitList);
	build_unit(main_filepath);
	link_project();
}
