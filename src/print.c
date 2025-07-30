#include <stdarg.h>
#include "crunchy.h"

int64_t print_token(Token *token);
int64_t print_type(Type *type);
int64_t print_expr(Expr *expr);
int64_t print_stmt(Stmt *stmt);

static int level = 0;
static FILE *fs = 0;
static EscapeMod escape_mods[256] = {};

static int hex2nibble(char hex)
{
	return
		hex >= 'a' && hex <= 'f' ? hex - 'a' + 10 :
		hex >= 'A' && hex <= 'F' ? hex - 'A' + 10 :
		hex >= '0' && hex <= '9' ? hex - '0' :
		0;
}

void set_print_file(FILE *new_fs)
{
	fs = new_fs;
}

void set_escape_mod(char chr, EscapeMod mod)
{
	escape_mods[(uint8_t)chr] = mod;
}

int64_t print(char *msg, ...)
{
	if(!fs) fs = stdout;
	va_list args;
	va_start(args, msg);
	int64_t printed_chars_count = 0;

	while(*msg) {
		if(*msg == '%') {
			msg ++;
			uint8_t index = *msg;

			if(escape_mods[index]) {
				escape_mods[index](args);
			}
			else if(*msg == '%') {
				fputc('%', fs);
				printed_chars_count ++;
			}
			else if(*msg == 'i') {
				printed_chars_count += fprintf(fs, "%li", va_arg(args, int64_t));
			}
			else if(*msg == 's') {
				printed_chars_count += fprintf(fs, "%s", va_arg(args, char*));
			}
			else if(*msg == 'S') {
				char *start = va_arg(args, char*);
				int64_t length = va_arg(args, int64_t);
				printed_chars_count += fwrite(start, 1, length, fs);
			}
			else if(*msg == 'c') {
				fputc(va_arg(args, int), fs);
				printed_chars_count ++;
			}
			else if(*msg == '>') {
				for(int i=0; i<level; i++) printed_chars_count += fprintf(fs, "\t");
			}
			else if(*msg == '+') {
				level ++;
			}
			else if(*msg == '-') {
				level --;
			}
			else if(*msg == 'n') {
				void *node = va_arg(args, void*);
				Kind *kind = node;

				if(*kind > STMT_KIND_START)
					printed_chars_count += print_stmt(node);
				else if(*kind > EXPR_KIND_START)
					printed_chars_count += print_expr(node);
				else if(*kind > TYPE_KIND_START)
					printed_chars_count += print_type(node);
				else
					printed_chars_count += print_token(node);
			}
			else if(*msg == '[') {
				msg ++;

				if(*msg == ']') {
					fprintf(fs, "\x1b[0m");
				}
				else {
					int r = hex2nibble(*msg++) * 0x11;
					int g = hex2nibble(*msg++) * 0x11;
					int b = hex2nibble(*msg++) * 0x11;
					fprintf(fs, "\x1b[38;2;%i;%i;%im", r, g, b);
				}
			}
		}
		else {
			fputc(*msg, fs);
			printed_chars_count ++;
		}

		msg ++;
	}

	va_end(args);
	return printed_chars_count;
}

int64_t print_token(Token *token)
{
	return print("%S", token->start, token->length);
}

void print_token_list(Token *tokens)
{
	for(Token *token = tokens; token->kind != TK_EOF; token ++) {
		print("%s ",
			token->kind == TK_BOF     ? "<BOF>     " :
			token->kind == TK_EOF     ? "<EOF>     " :
			token->kind == TK_INT     ? "<INT>     " :
			token->kind == TK_IDENT   ? "<IDENT>   " :
			token->kind == TK_STRING  ? "<STRING>  " :

			#define _(a) \
			token->kind == KW_ ## a ? "<KEYWORD> " :
			KEYWORDS
			#undef _

			#define _(a, b) \
			token->kind == PT_ ## b ? "<PUNCT>   " :
			PUNCTS
			#undef _

			"<unknown-token>"
		);

		print("%n\n", token);
	}
}

int64_t print_type(Type *type)
{
	switch(type->kind) {
		case TY_INT:
			return print("int");
		case TY_BOOL:
			return print("bool");
		case TY_STRING:
			return print("string");
		default:
			return print("<unknown-type>");
	}
}

int64_t print_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			return print("%i", expr->ival);
		case EX_BOOL:
			return print("%s", expr->ival ? "true" : "false");

		case EX_STRING: {
			int64_t printed_chars_count = 0;
			printed_chars_count += print("\"");

			for(int64_t i=0; i < expr->length; i++) {
				if(expr->chars[i] == '"') {
					printed_chars_count += print("\\\"");
				}
				else {
					printed_chars_count += print("%c", expr->chars[i]);
				}
			}

			printed_chars_count += print("\"");
			return printed_chars_count;
		} break;

		case EX_VAR:
			return print("%n", expr->ident);
		case EX_CAST:
			return print("%n(%n)", expr->type, expr->subexpr);
		case EX_BINOP:
			return print("(%n%n%n)", expr->left, expr->op, expr->right);
		default:
			return print("<unknown-expr:%i>", expr->kind);
	}
}

int64_t print_stmt(Stmt *stmt)
{
	int64_t printed_chars_count = 0;
	printed_chars_count += print("%>");

	switch(stmt->kind) {
		case ST_VARDECL:
			printed_chars_count += print("var %n", stmt->ident);
			if(stmt->type) printed_chars_count += print(" : %n", stmt->type);
			if(stmt->init) printed_chars_count += print(" = %n", stmt->init);
			printed_chars_count += print(";\n");
			break;
		case ST_FUNCDECL:
			printed_chars_count += print("function %n() {%+\n", stmt->ident);
			printed_chars_count += print_block(stmt->body);
			printed_chars_count += print("%-%>}\n");
			break;
		case ST_PRINT:
			printed_chars_count += print("print %n;\n", stmt->value);
			break;
		case ST_ASSIGN:
			printed_chars_count += print("%n = %n;\n", stmt->target, stmt->value);
			break;
		case ST_IF:
			printed_chars_count += print("if %n {%+\n", stmt->cond);
			printed_chars_count += print_block(stmt->body);
			printed_chars_count += print("%-%>}\n");

			if(stmt->else_body) {
				printed_chars_count += print("%>else {%+\n");
				printed_chars_count += print_block(stmt->else_body);
				printed_chars_count += print("%-%>}\n");
			}

			break;
		default:
			printed_chars_count += print("<unknown-stmt>\n");
			break;
	}

	return printed_chars_count;
}

int64_t print_stmts(Stmt *stmts)
{
	int64_t printed_chars_count = 0;

	for(Stmt *stmt = stmts; stmt; stmt = stmt->next)
		printed_chars_count += print_stmt(stmt);

	return printed_chars_count;
}

int64_t print_block(Block *block)
{
	int64_t printed_chars_count = 0;
	printed_chars_count += print("%># scope: ");

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl)
		printed_chars_count += print("%n; ", decl->ident);

	printed_chars_count += print("\n");
	printed_chars_count += print_stmts(block->stmts);
	return printed_chars_count;
}