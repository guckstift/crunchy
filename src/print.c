#include <stdarg.h>
#include "crunchy.h"

void print_token(Token *token);
void print_type(Type *type);
void print_expr(Expr *expr);
void print_stmt(Stmt *stmt);

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

void print(char *msg, ...)
{
	if(!fs) fs = stdout;
	va_list args;
	va_start(args, msg);

	while(*msg) {
		if(*msg == '%') {
			msg ++;
			uint8_t index = *msg;

			if(escape_mods[index]) {
				escape_mods[index](args);
			}
			else if(*msg == '%') {
				fputc('%', fs);
			}
			else if(*msg == 'i') {
				fprintf(fs, "%li", va_arg(args, int64_t));
			}
			else if(*msg == 's') {
				fprintf(fs, "%s", va_arg(args, char*));
			}
			else if(*msg == 'S') {
				char *start = va_arg(args, char*);
				int64_t length = va_arg(args, int64_t);
				fwrite(start, 1, length, fs);
			}
			else if(*msg == 'c') {
				fputc(va_arg(args, int), fs);
			}
			else if(*msg == '>') {
				for(int i=0; i<level; i++) fprintf(fs, "\t");
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
					print_stmt(node);
				else if(*kind > EXPR_KIND_START)
					print_expr(node);
				else if(*kind > TYPE_KIND_START)
					print_type(node);
				else
					print_token(node);
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
		}

		msg ++;
	}

	va_end(args);
}

void print_token(Token *token)
{
	print("%S", token->start, token->length);
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

void print_type(Type *type)
{
	switch(type->kind) {
		case TY_INT:
			print("int");
			break;
		case TY_BOOL:
			print("bool");
			break;
		case TY_STRING:
			print("string");
			break;
		default:
			print("<unknown-type>");
			break;
	}
}

void print_expr(Expr *expr)
{
	switch(expr->kind) {
		case EX_INT:
			print("%i", expr->ival);
			break;
		case EX_BOOL:
			print("%s", expr->ival ? "true" : "false");
			break;
		case EX_STRING:
			print("\"");

			for(int64_t i=0; i < expr->length; i++) {
				if(expr->chars[i] == '"') {
					print("\\\"");
				}
				else {
					print("%c", expr->chars[i]);
				}
			}

			print("\"");
			break;
		case EX_VAR:
			print("%n", expr->ident);
			break;
		case EX_CAST:
			print("%n(%n)", expr->type, expr->subexpr);
			break;
		case EX_BINOP:
			print("(%n%n%n)", expr->left, expr->op, expr->right);
			break;
		default:
			print("<unknown-expr:%i>", expr->kind);
			break;
	}
}

void print_stmt(Stmt *stmt)
{
	print("%>");

	switch(stmt->kind) {
		case ST_VARDECL:
			print("var %n", stmt->ident);
			if(stmt->type) print(" : %n", stmt->type);
			if(stmt->init) print(" = %n", stmt->init);
			print(";\n");
			break;
		case ST_PRINT:
			print("print %n;\n", stmt->value);
			break;
		case ST_ASSIGN:
			print("%n = %n;\n", stmt->target, stmt->value);
			break;
		case ST_IF:
			print("if %n {%+\n", stmt->cond);
			print_block(stmt->body);
			print("%-%>}\n");

			if(stmt->else_body) {
				print("%>else {%+\n");
				print_block(stmt->else_body);
				print("%-%>}\n");
			}

			break;
		default:
			print("<unknown-stmt>\n");
			break;
	}
}

void print_stmts(Stmt *stmts)
{
	for(Stmt *stmt = stmts; stmt; stmt = stmt->next)
		print_stmt(stmt);
}

void print_block(Block *block)
{
	print("%># scope: ");

	for(Stmt *decl = block->decls; decl; decl = decl->next_decl)
		print("%n; ", decl->ident);

	print("\n");
	print_stmts(block->stmts);
}