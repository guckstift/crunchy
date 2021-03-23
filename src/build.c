void lex_unit();
void parse_unit();
void analyze_unit();
void generate_code();
void dump_tokens();
void dump_ast();
Symbol *lookup(Token *ident);
void declare_import(Stmt *decl);

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

void chdir_in(char *path)
{
	char *last_slash = 0;
	
	for(char *x = path; *x; x ++) {
		if(*x == '/') {
			last_slash = x;
		}
	}
	
	if(last_slash) {
		*last_slash = 0;
		chdir(path);
		*last_slash = '/';
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

size_t unit_hash(char *str)
{
	size_t h = 0x2b992ddfa23249d6;
	
	while(*str) {
		h ^= *str;
		h ^= h << 2;
		h ^= h >> 5;
		h ^= h << 11;
		h ^= h >> 17;
		h ^= h << 23;
		h ^= h >> 31;
		str ++;
	}
	
	return h;
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
	cc_cmd = append_str(cc_cmd, "gcc -c -std=c99 -pedantic-errors -o ");
	cc_cmd = append_str(cc_cmd, unit->opath);
	
	if(unit == project->main)
		cc_cmd = append_str(cc_cmd, " -DCRUNCHY_MAIN");
	
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
	unit->hash = unit_hash(unit->abspath);
	chdir_in(unit->abspath);
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

Unit *find_unit(char *abspath)
{
	for(Unit *u = project->units->first; u; u = u->next) {
		if(strcmp(u->abspath, abspath) == 0)
			return u;
	}
	
	return 0;
}

Unit *do_import(char *path)
{
	char *abspath = get_abspath(path);
	Unit *import_unit = find_unit(abspath);
	
	if(import_unit == 0) {
		Unit *unit_bak = unit;
		Token *token_bak = token;
		char *filename_bak = filename;
		size_t line_bak = line;
		size_t pos_bak = pos;
		Scope *scope_bak = scope;
		
		build_unit(abspath);
		
		import_unit = unit;
		
		unit = unit_bak;
		token = token_bak;
		filename = filename_bak;
		line = line_bak;
		pos = pos_bak;
		scope = scope_bak;
		
		chdir_in(unit->abspath);
	}
	
	Scope *import_scope = import_unit->ast->scope;
	
	for(Symbol *sym = import_scope->first; sym; sym = sym->next) {
		Stmt *decl = sym->decl;
		
		if(decl->exported) {
			if(lookup(sym->ident))
				error(
					"imported symbol '%s' is already declared\n",
					sym->ident->text
				);
			
			declare_import(decl);
		}
	}
	
	return import_unit;
}

void link_project()
{
	project->exepath = get_cache_path(project->main->abspath, ".exe");
	char *cc_cmd = 0;
	cc_cmd = append_str(cc_cmd, "gcc -o ");
	cc_cmd = append_str(cc_cmd, project->exepath);
	
	for(Unit *u = project->units->first; u; u = u->next) {
		cc_cmd = append_str(cc_cmd, " ");
		cc_cmd = append_str(cc_cmd, u->opath);
	}
	
	if(system(cc_cmd) != 0)
		error("linking of objects failed");
}

void build_project(char *main_filepath)
{
	project = create(Project);
	project->units = create(UnitList);
	build_unit(main_filepath);
	link_project();
	printf("build complete\n");
}
