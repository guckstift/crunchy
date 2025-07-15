#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

typedef enum : uint8_t {
	TK_INVALID,
	TK_EOF,

	TK_INT,
	TK_IDENT,

	KW_var,
	KW_int,

	PT_EQUALS,
	PT_SEMICOLON,
	PT_COLON,
} TokenKind;

typedef struct {
	TokenKind kind;
	char *start;
	char *end;
	int64_t ival;
} Token;

typedef enum : uint8_t {
	TY_INVALID,

	TY_INT,
} TypeKind;

typedef struct {
	TypeKind kind;
} Type;

typedef enum : uint8_t {
	EX_INVALID,

	EX_INT,
} ExprKind;

typedef struct {
	ExprKind kind;
	Type *type;
	int64_t ival;
} Expr;

typedef enum : uint8_t {
	ST_INVALID,

	ST_VARDECL,
} StmtKind;

typedef struct {
	StmtKind kind;
	void *next;
	Token *ident;
	Type *type;
	Expr *init;
} Stmt;

void error(char *msg)
{
	printf("error: %s\n", msg);
	exit(EXIT_FAILURE);
}

char *load_text_file(char *file_name)
{
	FILE *fs = fopen(file_name, "rb");

	if(!fs) {
		error("could not open input file");
	}

	fseek(fs, 0, SEEK_END);
	long size = ftell(fs);
	rewind(fs);
	char *text = malloc(size + 1);
	text[size] = 0;
	fread(text, 1, size, fs);
	fclose(fs);
	return text;
}

int64_t lex(char *src, Token **tokens_out)
{
	Token *tokens = 0;
	int64_t count = 0;

	while(*src) {
		TokenKind kind = TK_INVALID;
		char *start = src;
		int64_t ival = 0;

		if(isspace(*src)) {
			src ++;
			continue;
		}
		else if(isdigit(*src)) {
			while(isdigit(*src)) {
				ival = ival * 10 + (*src - '0');
				src ++;
			}

			kind = TK_INT;
		}
		else if(isalpha(*src)) {
			while(isalpha(*src) || isdigit(*src)) {
				src ++;
			}

			int64_t length = src - start;

			if(length == 3 && memcmp(start, "var", length) == 0) {
				kind = KW_var;
			}
			else if(length == 3 && memcmp(start, "int", length) == 0) {
				kind = KW_int;
			}
			else {
				kind = TK_IDENT;
			}
		}
		else if(*src == '=') {
			kind = PT_EQUALS;
			src ++;
		}
		else if(*src == ';') {
			kind = PT_SEMICOLON;
			src ++;
		}
		else if(*src == ':') {
			kind = PT_COLON;
			src ++;
		}
		else {
			printf("(%i) %c\n", *src, *src);
			error("unrecognized token");
		}

		count ++;
		tokens = realloc(tokens, sizeof(Token) * count);
		tokens[count - 1] = (Token){.kind = kind, .start = start, .end = src, .ival = ival};
	}

	count ++;
	tokens = realloc(tokens, sizeof(Token) * count);
	tokens[count - 1] = (Token){.kind = TK_EOF, .start = src, .end = src};

	*tokens_out = tokens;
	return count;
}

Token *cur_token = 0;

Token *eat(TokenKind kind)
{
	if(cur_token->kind == kind) {
		return cur_token ++;
	}

	return 0;
}

Type *p_type()
{
	Token *keyword = eat(KW_int);

	if(!keyword) {
		return 0;
	}

	Type *type = malloc(sizeof(Type));
	type->kind = TY_INT;
	return type;
}

Expr *p_expr()
{
	Token *int_literal = eat(TK_INT);

	if(!int_literal) {
		return 0;
	}

	Expr *expr = malloc(sizeof(Expr));
	expr->kind = EX_INT;
	expr->ival = int_literal->ival;
	return expr;
}

Stmt *p_stmt()
{
	if(!eat(KW_var)) {
		return 0;
	}

	Token *ident = eat(TK_IDENT);

	if(!ident) {
		error("missing variable name after var keyword");
	}

	Type *type = 0;

	if(eat(PT_COLON)) {
		type = p_type();

		if(!type) {
			error("missing type specification after colon");
		}
	}

	Expr *init = 0;

	if(eat(PT_EQUALS)) {
		init = p_expr();

		if(!init) {
			error("missing expression after equals");
		}
	}

	if(!type && !init) {
		error("a variable declaration must have at least either a type specification or an initializer expression");
	}

	if(!eat(PT_SEMICOLON)) {
		error("missing semicolon after variable declaration");
	}

	Stmt *stmt = malloc(sizeof(Stmt));
	stmt->kind = ST_VARDECL;
	stmt->next = 0;
	stmt->ident = ident;
	stmt->type = type;
	stmt->init = init;
	return stmt;
}

Stmt *parse(Token *tokens)
{
	cur_token = tokens;

	Stmt *first = 0;
	Stmt *last = 0;

	while(1) {
		Stmt *stmt = p_stmt();

		if(!stmt) {
			break;
		}

		if(!first) {
			first = stmt;
			last = stmt;
		}
		else {
			last->next = stmt;
			last = stmt;
		}
	}

	if(!eat(TK_EOF)) {
		error("invalid statement");
	}

	return first;
}

void a_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			expr->type = malloc(sizeof(Type));
			expr->type->kind = TY_INT;
			break;
	}
}

void a_stmt(Stmt *stmt)
{
	switch(stmt->kind) {
		case ST_VARDECL:
			if(stmt->init) {
				a_expr(stmt->init);

				if(!stmt->type) {
					stmt->type = stmt->init->type;
				}
			}
			else if(stmt->type) {
				stmt->init = malloc(sizeof(Expr));
				stmt->init->kind = EX_INT;
				stmt->init->type = stmt->type;
				stmt->init->ival = 0;
			}

			break;
	}
}

void analyse(Stmt *stmts)
{
	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		a_stmt(stmt);
	}
}

FILE *ofs = 0;

void gen_type(Type *type)
{
	switch(type->kind) {
		case TY_INT:
			fprintf(ofs, "int64_t");
			break;
	}
}

void gen_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			fprintf(ofs, "%li", expr->ival);
			break;
	}
}

void gen_stmt(Stmt *stmt)
{
	fprintf(ofs, "\t");

	switch(stmt->kind) {
		case ST_VARDECL:
			fwrite(stmt->ident->start, 1, stmt->ident->end - stmt->ident->start, ofs);
			fprintf(ofs, " = ");
			gen_expr(stmt->init);
			fprintf(ofs, ";\n");
			break;
	}
}

void generate(Stmt *stmts, char *output_file)
{
	ofs = fopen(output_file, "wb");
	fprintf(ofs, "#include <stdint.h>\n");

	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		if(stmt->kind == ST_VARDECL) {
			gen_type(stmt->type);
			fprintf(ofs, " ");
			fwrite(stmt->ident->start, 1, stmt->ident->end - stmt->ident->start, ofs);
			fprintf(ofs, ";\n");
		}
	}

	fprintf(ofs, "int main(int argc, char **argv) {\n");

	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		gen_stmt(stmt);
	}

	fprintf(ofs, "\treturn 0;\n");
	fprintf(ofs, "}\n");
	fclose(ofs);
}

void print_type(Type *type)
{
	switch(type->kind) {
		case TY_INT:
			printf("int");
			break;
	}
}

void print_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			printf("%li", expr->ival);
			break;
	}
}

void print_stmt(Stmt *stmt)
{
	switch(stmt->kind) {
		case ST_VARDECL:
			printf("var ");
			fwrite(stmt->ident->start, 1, stmt->ident->end - stmt->ident->start, stdout);

			if(stmt->type) {
				printf(" : ");
				print_type(stmt->type);
			}

			if(stmt->init) {
				printf(" = ");
				print_expr(stmt->init);
			}

			printf(";\n");
			break;
	}
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		error("missing input file parameter");
	}

	char *input_file = argv[1];
	char *src = load_text_file(input_file);

	printf("content: %s \n", src);

	Token *tokens = 0;
	int64_t token_count = lex(src, &tokens);

	for(Token *token = tokens; token->kind != TK_EOF; token ++) {
		printf("%s ",
			token->kind == TK_INT       ? "<INT>       " :
			token->kind == TK_IDENT     ? "<IDENT>     " :
			token->kind == KW_var       ? "<KEYWORD>   " :
			token->kind == PT_EQUALS    ? "<EQUALS>    " :
			token->kind == PT_SEMICOLON ? "<SEMICOLON> " :
			token->kind == PT_COLON     ? "<COLON>     " :
			"<invalid-token>"
		);

		fwrite(token->start, 1, token->end - token->start, stdout);
		printf("\n");
	}

	Stmt *stmts = parse(tokens);

	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		print_stmt(stmt);
	}

	analyse(stmts);

	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		print_stmt(stmt);
	}

	int64_t input_filename_length = strlen(input_file);
	char *output_file = malloc(input_filename_length + 2 + 1);
	memcpy(output_file, input_file, input_filename_length);
	memcpy(output_file + input_filename_length, ".c", 2);
	output_file[input_filename_length + 2] = 0;

	generate(stmts, output_file);

	return 0;
}