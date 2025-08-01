#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef enum : uint8_t {
	TY_STRING,
	TY_ARRAY,
} Type;

typedef struct {
	void *next;
	Type type;
	uint8_t marked;
} MemoryBlock;

typedef struct {
	MemoryBlock block;
	int64_t length;
	char chars[];
} String;

typedef struct {
	MemoryBlock block;
	int64_t length;
	void *items;
} Array;

typedef struct {
	void *parent;
	int64_t num_gc_decls;
	MemoryBlock *gc_objs[];
} Frame;

typedef void (*Function)(void);

void noop(void){}

static MemoryBlock *memory_blocks = 0;
static Frame *cur_frame = 0;

void collect_garbage()
{
	for(Frame *frame = cur_frame; frame; frame = frame->parent) {
		for(int64_t i=0; i < frame->num_gc_decls; i++) {
			MemoryBlock *gc_obj = frame->gc_objs[i];
			if(gc_obj) gc_obj->marked = 1;
		}
	}

	MemoryBlock *block = memory_blocks;
	MemoryBlock *prev = 0;

	while(block) {
		MemoryBlock *next = block->next;

		if(!block->marked) {
			if(prev) prev->next = block->next;
			else memory_blocks = block->next;
			free(block);
			//printf("## collected %s\n", ((String*)block)->chars);
		}
		else {
			block->marked = 0;
			prev = block;
		}

		block = next;
	}
}

void *new_memory_block(Type type, int64_t size)
{
	collect_garbage();
	MemoryBlock *block = malloc(size);
	block->next = memory_blocks;
	block->type = type;
	block->marked = 0;
	memory_blocks = block;
	return block;
}

String *new_string(int64_t length, char *chars)
{
	int64_t size = sizeof(String) + length + 1;
	String *string = new_memory_block(TY_STRING, size);
	string->length = length;
	memcpy(string->chars, chars, length);
	string->chars[length] = 0;
	//printf("## created %s\n", string->chars);
	return string;
}

Array *new_array(int64_t itemsize, int64_t length, void *data)
{
	Array *array = new_memory_block(TY_ARRAY, sizeof(Array));
	array->length = length;
	int64_t data_size = itemsize * length;
	array->items = calloc(length, itemsize);
	memcpy(array->items, data, data_size);
	return array;
}

void print_string(String *str)
{
	fwrite(str->chars, 1, str->length, stdout);
}

String *concat_strings(String *left, String *right)
{
	int64_t length = left->length + right->length;
	int64_t size = sizeof(String) + length + 1;
	String *string = new_memory_block(TY_STRING, size);
	string->length = length;
	memcpy(string->chars, left->chars, left->length);
	memcpy(string->chars + left->length, right->chars, right->length);
	string->chars[length] = 0;
	//printf("## concated %s\n", string->chars);
	return string;
}