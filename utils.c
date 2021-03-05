#include "crunchy.h"

char *read_file(char *filename)
{
	FILE *filestream = fopen(filename, "rb");
	
	if(filestream == 0) {
		char *empty = malloc(1);
		*empty = 0;
		return empty;
	}
	
	fseek(filestream, 0, SEEK_END);
	long filesize = ftell(filestream);
	char *content = malloc(filesize + 1);
	fseek(filestream, 0, SEEK_SET);
	fread(content, 1, filesize, filestream);
	fclose(filestream);
	content[filesize] = 0;
	return content;
}

char *clone_substring(char *start, size_t length)
{
	char *cloned = malloc(length + 1);
	memcpy(cloned, start, length);
	cloned[length] = 0;
	return cloned;
}

size_t match_substring(char *start, char *substring)
{
	size_t i = 0;
	
	while(start[i] && substring[i] && start[i] == substring[i]) {
		i ++;
	}
	
	if(substring[i] == 0) {
		return i;
	}
	
	return 0;
}
