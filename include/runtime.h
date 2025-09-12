#include "crunchy.h"

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

extern Type t_int;
extern Type t_bool;
extern Type t_string;
extern Type t_func;

void push_frame(void *frame);
void pop_frame();
void *get_cur_frame();
String *new_string(int64_t length, char *chars);
Array *new_array(Type *type, int64_t length, void *data);
void print_string(String *str);
void print_array(Array *array, Type *type);
String *concat_strings(String *left, String *right);