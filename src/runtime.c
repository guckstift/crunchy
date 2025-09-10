#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef enum : uint8_t {
	TY_UNKNOWN,
	TY_VOID,
	TY_INT,
	TY_BOOL,
	TY_STRING,
	TY_FUNC,
	TY_ARRAY,
} TypeKind;

typedef struct Type {
	TypeKind kind;
	struct Type *subtype;
} Type;

typedef struct {
	void *next;
	Type *type;
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

static Type t_int = {.kind = TY_INT};
static Type t_bool = {.kind = TY_BOOL};
static Type t_string = {.kind = TY_STRING};
static Type t_func = {.kind = TY_FUNC};

static MemoryBlock *memory_blocks = 0;
static Frame *cur_frame = 0;

void mark_array(Array *array, Type *type)
{
	for(int64_t i=0; i < array->length; i++) {
		if(type->subtype->kind == TY_STRING) {
			String *str = ((String**)array->items)[i];
			str->block.marked = 1;
		}
		else if(type->subtype->kind == TY_ARRAY) {
			Array *arr = ((Array**)array->items)[i];
			arr->block.marked = 1;
			mark_array(arr, type->subtype);
		}
	}
}

void collect_garbage()
{
	for(Frame *frame = cur_frame; frame; frame = frame->parent) {
		for(int64_t i=0; i < frame->num_gc_decls; i++) {
			MemoryBlock *gc_obj = frame->gc_objs[i];

			if(gc_obj && !gc_obj->marked) {
				gc_obj->marked = 1;

				if(gc_obj->type->kind == TY_ARRAY) {
					Array *array = (void*)gc_obj;
					mark_array(array, gc_obj->type);
				}
			}
		}
	}

	MemoryBlock *block = memory_blocks;
	MemoryBlock *prev = 0;

	while(block) {
		MemoryBlock *next = block->next;

		if(!block->marked) {
			if(block->type->kind == TY_ARRAY) {
				Array *array = (Array*)block;
				free(array->items);
			}

			if(prev) prev->next = block->next;
			else memory_blocks = block->next;
			free(block);
			//printf("## collected %p \n", block);
		}
		else {
			block->marked = 0;
			prev = block;
		}

		block = next;
	}
}

void *new_memory_block(Type *type, int64_t size)
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
	String *string = new_memory_block(&t_string, size);
	string->length = length;
	memcpy(string->chars, chars, length);
	string->chars[length] = 0;
	//printf("## created %s %p \n", string->chars, string);
	return string;
}

Array *new_array(Type *type, int64_t length, void *data)
{
	int64_t itemsize =
		type->kind == TY_INT ? sizeof(int64_t) :
		type->kind == TY_BOOL ? sizeof(uint8_t) :
		type->kind == TY_INT ? sizeof(String*) :
		type->kind == TY_FUNC ? sizeof(Function) :
		type->kind == TY_ARRAY ? sizeof(Array*) :
		sizeof(void*);

	Array *array = new_memory_block(type, sizeof(Array));
	array->length = length;
	int64_t data_size = itemsize * length;
	array->items = calloc(length, itemsize);
	memcpy(array->items, data, data_size);
	//printf("## created array with %li elms %p \n", length, array);
	return array;
}

void print_string(String *str)
{
	fwrite(str->chars, 1, str->length, stdout);
}

void print_array(Array *array, Type *type)
{
	printf("[");

	for(int64_t i=0; i < array->length; i++) {
		if(i > 0) printf(", ");

		switch(type->subtype->kind) {
			case TY_INT:
				printf("%li", ((int64_t*)array->items)[i]);
				break;
			case TY_BOOL:
				printf("%s", ((uint8_t*)array->items)[i] ? "true" : "false");
				break;
			case TY_STRING:
				print_string(((String**)array->items)[i]);
				break;
			case TY_FUNC:
				printf("<Function>");
				break;
			case TY_ARRAY:
				print_array(((Array**)array->items)[i], type->subtype);
				break;
		}
	}

	printf("]");
}

String *concat_strings(String *left, String *right)
{
	int64_t length = left->length + right->length;
	int64_t size = sizeof(String) + length + 1;
	String *string = new_memory_block(&t_string, size);
	string->length = length;
	memcpy(string->chars, left->chars, left->length);
	memcpy(string->chars + left->length, right->chars, right->length);
	string->chars[length] = 0;
	//printf("## concated %s\n", string->chars);
	return string;
}