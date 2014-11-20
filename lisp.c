/*
 * Copyright (c) 2014, Sean Anderson All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#ifndef WITH_EDITLINE

static char buffer[2048];

char* readline(char* prompt) {

	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;

}

void add_history(char* unused) {}

#else

#include <editline/readline.h>

#endif // WITH_EDITLINE

#include "mpc.h"
#include "lisp.h"

mpc_parser_t* Number;
mpc_parser_t* Boolean;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Symbol;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Lisp;


int main(int argc, char** argv){

	//Init the parser
	Number	= mpc_new("number");
	Boolean	= mpc_new("boolean");
	String	= mpc_new("string");
	Comment	= mpc_new("comment");
	Symbol	= mpc_new("symbol");
	Sexpr	= mpc_new("sexpr");
	Qexpr	= mpc_new("qexpr");
	Expr	= mpc_new("expr");
	Lisp	= mpc_new("lisp");

	mpca_lang(MPCA_LANG_DEFAULT,
	"\
	number		: /-?[0-9]+/ ;							\
	boolean		: \"true\" | \"false\" ;				\
	string	: /\"(\\\\.|[^\"])*\"/ ;				\
	comment		: /;[^\\r\\n]*/ ;						\
	symbol		: /[a-zA-Z0-9_+\\-%*\\/\\\\=<>!&]+/ ;		\
	qexpr		: '{' <expr>* '}' ;						\
	sexpr		: '(' <expr>* ')' ;						\
	expr		: <number> | <boolean> | <string> |		\
			<comment> | <symbol> | <sexpr> | <qexpr>;	\
	lisp		: /^/ <expr>* /$/ ;						\
	", Number, Boolean, String, Comment, Symbol, Qexpr, Sexpr, Expr, Lisp);

	//Init the env
	lenv* e = lenv_new(LENV_INIT);
	lenv_add_builtins(e);

	//Read in files
	if(argc >= 2) {
		for(int i = 1; i < argc; i++){
			lval* args = lval_append(lval_sexp(), lval_str(argv[i]));
			lval* err = builtin_load(e, args);
			if(err->type == LVAL_ERR) lval_println(err);
			lval_del(err);
		}
	}

	if(!isatty(fileno(stdin))) {  //Check to see if we are in an interactive shell
		lval* args = lval_append(lval_sexp(), lval_str("stdin"));
		lval* err = builtin_load(e, args);
		if(err->type == LVAL_ERR) lval_println(err);
		lval_del(err);
		return 0;
	}

	//Version and exit info
	puts("Lisp Version 0.1.0");
	puts("Ctrl-C or (exit 0) to exit\n");
	//printf("%i", sizeof(lval));

	while(1) {

		//Read
		char* input = readline("lisp> ");
		add_history(input);

		if(input == NULL) break;  //Check for a null input

		mpc_result_t r;
		if(!(mpc_parse("<stdin>", input, Lisp, &r))){
			//Failure to parse the input
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
			free(input);
			continue;
		}

		lval* tree = lval_eval(e, lval_read(r.output));
		lval_println(tree);
		lval_del(tree);
		mpc_ast_delete(r.output);

		free(input);

	}

	lenv_del(e);

	mpc_cleanup(9, Number, Boolean, String, Comment, Symbol, Sexpr, Qexpr, Expr, Lisp);
	return 0;

}

//Create an lval from a given number
lval* lval_num(const long num){

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = num;
	return v;

}

//Create an lval from a given error string
lval* lval_err(char* fmt, ...){

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;

	va_list va;
	va_start(va, fmt);
	v->str = malloc(LVAL_ERR_MAX);
	vsnprintf(v->str, LVAL_ERR_MAX - 1, fmt, va);
	v->str = realloc(v->str, strlen(v->str) + 1);  //Free excess space in the string
	va_end(va);
	return v;

}

lval* lval_str(char* str){

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_STR;
	v->str = malloc(strlen(str) + 1);
	strcpy(v->str, str);
	return v;

}

//A sym lval from the message
lval* lval_sym(char* message){

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->str = malloc(strlen(message) + 1);
	strcpy(v->str, message);
	return v;

}

//An empty sexp
lval* lval_sexp(){

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;

}

lval* lval_qexpr() {

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;

}

lval* lval_func(lbuiltin func){

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUNC;
	v->builtin = func;
	return v;

}

lval* lval_lambda(lval* formals, lval* body) {

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUNC;
	v->builtin = NULL;
	v->env  = lenv_new(LENV_LOCAL_INIT);
	v->formals = formals;
	v->body = body;
	return v;

}

//Booleans are immutable, so we only return pointers to LVAL_TRUE or LVAL_FALSE
lval* lval_bool(int boolean) {

	if(boolean)
		return LVAL_TRUE;
	else
		return LVAL_FALSE;

}

void lval_del(lval* v){

	switch(v->type){
		case(LVAL_NUM): break;
		case(LVAL_BOOL): return;  //Can't free an immutable
		case(LVAL_FUNC): if(!v->builtin) {
			lenv_del(v->env);
			lval_del(v->formals);
			lval_del(v->body);
		} break;
		case(LVAL_STR): free(v->str); break;
		case(LVAL_ERR): free(v->str); break;
		case(LVAL_SYM): free(v->str); break;
		case(LVAL_SEXPR):
		case(LVAL_QEXPR):
			for(int i = 0; i < v->count; i++)  //Free the array of lvals
				lval_del(v->cell[i]);
			free(v->cell);
			break;
	}
	free(v);
}

//Append element to v
lval* lval_append(lval* v, lval* element){

		v->count++;
		v->cell = realloc(v->cell, sizeof(lval*) * v->count);
		v->cell[v->count - 1] = element;
		return v;

}

//Remove an sexpr at index from v and return it
lval* lval_pop(lval* v, int index) {

	lval* pop = v->cell[index];

	//Copy over the space pop was using
	memmove(&v->cell[index], &v->cell[index+1], sizeof(lval*) * (v->count-index-1));

	v->count--;
	//Resize the array
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return pop;

}

//Get rid of v and return the element at index
lval* lval_take(lval* v, int index){

	lval* pop = lval_pop(v, index);
	lval_del(v);
	return pop;
}

//Tests to see if two lvals are equal (identical)
lval* lval_equals(lval* x, lval* y) {

	if(x == y) return LVAL_TRUE;  //This takes care of booleans, since immutibility
	if(x->type != y->type) return LVAL_FALSE;  //TODO: should we error on this?

	switch(x->type) {
		case(LVAL_BOOL): break;
		case(LVAL_NUM): if(x->num == y->num) return LVAL_TRUE; break;
		case(LVAL_FUNC): if(x->builtin) {
			if(x->builtin == y->builtin) return LVAL_TRUE; break;
		} else {
			if((lenv_equals(x->env, y->env) == LVAL_TRUE) &&
			(lval_equals(x->formals, y->formals) == LVAL_TRUE) &&
			(lval_equals(x->body, y->body) == LVAL_TRUE)) return LVAL_TRUE; break;
		}
		case(LVAL_ERR): if(strcmp(x->str, y->str) == 0) return LVAL_TRUE; break;
		case(LVAL_SYM): if(strcmp(x->str, y->str) == 0) return LVAL_TRUE; break;
		case(LVAL_STR): if(strcmp(x->str, y->str) == 0) return LVAL_TRUE; break;
		case(LVAL_QEXPR):
		case(LVAL_SEXPR): if(x->count != y->count) break;
			for(int i = 0; i < x->count; i++) {
				if(lval_equals(x->cell[i], y->cell[i]) == LVAL_FALSE) break;
			} return LVAL_TRUE;  //Everything equals
	}
	return LVAL_FALSE;

}

//Compare 2 numbers
/*rel lval_compare(lval* x, lval* y) {

	if(x->num > y->num) return GT;
	else if(x->num == y->num) return EQ;
	else return LT;

}*/

lval* lval_join(lval* x, lval* y) {

	//Add all the cells in y to x
	while(y->count) x = lval_append(x, lval_pop(y, 0));

	lval_del(y);
	return x;

}

lval* lval_copy(lval* v) {

	lval* x = malloc(sizeof(lval));
	x->type = v->type;

	switch(v->type) {
		case(LVAL_NUM): x->num = v->num; break;
		case(LVAL_BOOL): free(x); return v;
		case(LVAL_FUNC): if(v->builtin) {
			x->builtin = v->builtin;
		} else {
			x->builtin = NULL;
			x->env = lenv_copy(v->env);
			x->formals = lval_copy(v->formals);
			x->body = lval_copy(v->body);
		} break;
		case(LVAL_ERR): x->str = malloc(strlen(v->str) + 1); strcpy(x->str, v->str); break;
		case(LVAL_SYM): x->str = malloc(strlen(v->str) + 1); strcpy(x->str, v->str); break;
		case(LVAL_STR): x->str = malloc(strlen(v->str) + 1); strcpy(x->str, v->str); break;
		case(LVAL_SEXPR):
		case(LVAL_QEXPR):
			x->count = v->count;
			x->cell = malloc(sizeof(lval*) * v->count);
			for(int i = 0; i < v->count; i++)
				x->cell[i] = lval_copy(v->cell[i]);
			break;
	}

	return x;
}

void lval_print(lval* v){

	switch(v->type){
		case(LVAL_BOOL): if(v == LVAL_TRUE) printf("true");
			else printf("false"); break;
		case(LVAL_NUM): printf("%li", v->num); break;
		case(LVAL_ERR): printf("Error: %s", v->str); break;
		case(LVAL_SYM): printf("%s", v->str); break;
		case(LVAL_STR): lval_str_print(v); break;
		case(LVAL_FUNC): if(v->builtin) {
			printf("<builtin>");
		} else {
			printf("(\\ "); lval_print(v->formals);
			putchar(' ');
			lval_print(v->body); putchar(')');
		} break;
		case(LVAL_SEXPR): lval_expr_print(v, '(', ')'); break;
		case(LVAL_QEXPR): lval_expr_print(v, '{', '}'); break;
	}

}

void lval_expr_print(lval* v, char open, char close){

	putchar(open);
	for(int i = 0; i < v->count; i++){
		//Print the value
		lval_print(v->cell[i]);
		//Only put a space if it's not last
		if(i != (v->count - 1)) putchar(' ');
	}
	putchar(close);

}

void lval_str_print(lval* v){

	char* escaped = malloc(strlen(v->str) + 1);
	strcpy(escaped, v->str);

	escaped = mpcf_escape(escaped);
	printf("\"%s\"", escaped);
	free(escaped);

}

void lval_println(lval* v){
	lval_print(v); putchar('\n');
}

//Read a number from an abstract syntax tree
lval* lval_read_num(mpc_ast_t* t){

	errno = 0;
	long num = strtol(t->contents, NULL, 10);  //Base 10
	return errno != ERANGE ? lval_num(num) : lval_err("invalid number");

}

lval* lval_read_str(mpc_ast_t* t){

	//Replace the last " with \0
	t->contents[strlen(t->contents)-1] = '\0';
	//Copy w/o the first '
	char* unescaped = malloc(strlen(t->contents + 1) + 1);
	strcpy(unescaped, t->contents + 1);

	unescaped = mpcf_unescape(unescaped);
	lval* str = lval_str(unescaped);
	free(unescaped);
	return str;

}

lval* lval_read(mpc_ast_t* t){

	//mpc_ast_print(t);

	//Return if it's not an sexpr or qexpr
	if(strstr(t->tag, "number")) return lval_read_num(t);
	if(strstr(t->tag, "symbol")) return lval_sym(t->contents);
	if(strstr(t->tag, "|string|")) return lval_read_str(t);
	if(strstr(t->tag, "boolean")) {
		if(strstr(t->contents, "true")) return LVAL_TRUE;
		else return LVAL_FALSE;
	}

	//It should be an ?expr or the root
	lval* tree = NULL;
	//Check to be sure
	if((strcmp(t->tag, ">") == 0) || strstr(t->tag, "sexpr")) tree = lval_sexp();
	if(strstr(t->tag, "qexpr")) tree = lval_qexpr();

	for(int i = 0; i < t->children_num; i++){
		#define IF_NOT(string) if(strcmp(t->children[i]->contents, string) == 0) continue;
		IF_NOT("(")
		IF_NOT(")")
		IF_NOT("{")
		IF_NOT("}")
		#undef IF_NOT
		if(strcmp(t->children[i]->tag, "regex") == 0) continue;
		if(strstr(t->children[i]->tag, "comment")) continue;
		tree = lval_append(tree, lval_read(t->children[i]));
	}

	return tree;

}

//Evaluate an sexpr
lval* lval_eval_sexpr(lenv* e, lval* v){

	//Evaluate children
	for(int i = 0; i < v->count; i++){
		v->cell[i] = lval_eval(e, v->cell[i]);
		if(v->cell[i]->type == LVAL_ERR) return lval_take(v, i);
	}

	//Deal with empty/1 value sexprs
	if(v->count == 0) return v;
	if(v->count == 1) return lval_eval(e, lval_take(v, 0));

	//Make sure we have a function
	lval* func = lval_pop(v, 0);
	if(func->type != LVAL_FUNC){
		lval* err = lval_err("S-Expression does not start with a function: got %s, expected %s", ltype_name(func->type), ltype_name(LVAL_FUNC));
		lval_del(func);
		lval_del(v);
		return err;
	}

	lval* result = lval_call(e, func, v);
	lval_del(func);  //builtin_op deletes v
	return result;

}

lval* lval_eval(lenv* e, lval* v) {

	if(v->type == LVAL_SYM){
		lval* x = lenv_get(e, v);
		lval_del(v);
		//return lval_eval(e, x);
		return x;
	}

	//eval sexprs
	if(v->type == LVAL_SEXPR) return lval_eval_sexpr(e, v);
	//And everything else
	return v;
}

lval* lval_call(lenv* e, lval* func, lval* args) {

	if(func->builtin) return func->builtin(e, args);

	int given = args->count;
	int total = func->formals->count;

	while(args->count) {
		if(func->formals->count == 0) {
			lval_del(args);
			return lval_err("Function passed too many arguments: got %i, expected %i", given, total);
		}

		//Define the func's arguments to be what was passed
		lval* sym = lval_pop(func->formals, 0);

		//Special variable-arguments case
		if(strcmp(sym->str, "&") == 0) {
			if(func->formals->count != 1) {  //& must be followed by exactly one symbol
				lval_del(args);
				return lval_err("Function format invalid: symbol \"&\" not followed by exactly one symbol");
			}

			lval* nsym = lval_pop(func->formals, 0);
			lenv_put(func->env, nsym, builtin_list(e, args));  //Bind the last formal to a list of the remaining args
			lval_del(sym);
			lval_del(nsym);
			break;
		}

		lval* val = lval_pop(args, 0);
		lenv_put(func->env, sym, val);
		lval_del(val);
		lval_del(sym);
	}

	lval_del(args);

	//If we have unpassed variable arguments
	if(func->formals->count > 0 && strcmp(func->formals->cell[0]->str, "&") == 0) {
		if(func->formals->count != 2) //& has to have a list to bind
			return lval_err("Function format invalid: symbol \"&\" not followed by exactly one symbol");

		lval_del(lval_pop(func->formals, 0));  //Pop the &

		lval* sym = lval_pop(func->formals, 0);
		lval* val = lval_qexpr();
		lenv_put(func->env, sym, val);
		lval_del(sym);
		lval_del(val);

	}

	if(func->formals->count == 0) {  //Evaluate and return
		func->env->par = e;
		return builtin_eval(func->env, lval_append(lval_sexp(), lval_copy(func->body)));
	} else  //Or just return the .5 eval'd func
		return lval_copy(func);
}

char* ltype_name(enum ltype type){

	switch(type) {
		case(LVAL_FUNC): return "Function";
		case(LVAL_NUM): return "Number";
		case(LVAL_BOOL): return "Boolean";
		case(LVAL_ERR): return "Error";
		case(LVAL_SYM): return "Symbol";
		case(LVAL_SEXPR): return "S-Expression";
		case(LVAL_QEXPR): return "Q-Expression";
	}

}

//A helper assertion function
#define LASSERT(args, cond, fmt, ...) do { if(cond) { lval* err = lval_err(fmt, __VA_ARGS__); lval_del(args); return err;} } while(false)
//Check if we have the right type (arg is the number of the argument)
#define LASSERT_TYPE(args, func, arg, type, expected) do { LASSERT((args), ((type) != (expected)), "Function \"%s\" passed incorrect type for argument %i: got %s, expected %s", (func), (arg), ltype_name(type), ltype_name(expected)); } while(false)
//Check if we got the right number of args
#define LASSERT_ARGS(args, func, num_args, expected) do { LASSERT((args), ((num_args) != (expected)), "Function \"%s\" passed wrong number of args: got %i, expected %i", (func), (num_args), (expected)); } while(false)
//Check if the list is empty
#define LASSERT_EMPTY(args, func, list) do { LASSERT((args), ((list)->count == 0), "Function \"%s\" passed empty %s", (func), ltype_name((list)->type)); } while(false)

lval* builtin_op(lenv* e, lval* args, char* op){

	for(int i = 0; i < args->count; i++)
		LASSERT_TYPE(args, op, i, args->cell[i]->type, LVAL_NUM);

	lval* first = lval_pop(args, 0);

	if((strcmp(op, "-") == 0) && args->count == 0) first->num = -first->num;

	while(args->count > 0){
		lval* second = lval_pop(args, 0);

		//Helper macro for C operators
		#define ADD_OP(operator) do{ if(strcmp(op, #operator) == 0) first->num = first->num operator second->num; } while(false)
		ADD_OP(+);
		ADD_OP(-);
		ADD_OP(*);
		ADD_OP(%);
		#undef ADD_OP

		//Helper macro for functions in the format "long func(long, long)"
		#define ADD_OP(operator, function) do{ if(strcmp(op, #operator) == 0) first->num = function(first->num, second->num); } while(false)
		ADD_OP(^, pow);
		ADD_OP(min, fmin);
		ADD_OP(max, fmax);
		#undef ADD_OP

		//And divide gets its own thing
		if(strcmp(op, "/") == 0) {
			if(second->num == 0) {
				lval_del(first);
				lval_del(second);
				first = lval_err("Division by zero");
				break;  //Exit the while loop
			}
			first->num /= second->num;
		}

		lval_del(second);

	}

	lval_del(args);
	return first;

}

//Return the first element in a list
lval* builtin_head(lenv* e, lval* args) {

	LASSERT_ARGS(args, "head", args->count, 1);
	LASSERT_TYPE(args, "head", 0, args->cell[0]->type, LVAL_QEXPR);
	LASSERT_EMPTY(args, "head", args->cell[0]);

	lval* v;
	for(v = lval_take(args, 0);
		v->count > 1;
		lval_del(lval_pop(v, 1)));  //Delete everything until we have 1 argument left

	return v;

}

//Return the last elements of the list
lval* builtin_tail(lenv* e, lval* args) {

	LASSERT_ARGS(args, "tail", args->count, 1);
	LASSERT_TYPE(args, "tail", 0, args->cell[0]->type, LVAL_QEXPR);
	LASSERT_EMPTY(args, "tail", args->cell[0]);

	lval* v = lval_take(args, 0);
	lval_del(lval_pop(v, 0));
	return v;

}

//"Convert" an sexpr to a qexpr
lval* builtin_list(lenv* e, lval* args) {

	args->type = LVAL_QEXPR;
	return args;

}

//Evaluate a qexpr
lval* builtin_eval(lenv* e, lval* args) {

	LASSERT_ARGS(args, "eval", args->count, 1);
	LASSERT_TYPE(args, "eval", 0, args->cell[0]->type, LVAL_QEXPR);

	lval* v = lval_take(args, 0);
	v->type = LVAL_SEXPR;
	return lval_eval(e, v);

}

lval* builtin_join(lenv* e, lval* args) {

	for(int i = 0; i < args->count; i++) {
			LASSERT_TYPE(args, "join", i, args->cell[i]->type, LVAL_QEXPR);
	}

	lval* x;
	for(x = lval_pop(args, 0); args->count; lval_join(x, lval_pop(args, 0)));  //Join the arguments together

	lval_del(args);
	return x;

}

typedef enum var {VAR_DEF, VAR_PUT} var;

static lval* builtin_var(lenv* e, lval* args, var func) {

	LASSERT_TYPE(args, "def", 0, args->cell[0]->type, LVAL_QEXPR);

	lval* syms = args->cell[0];

	for(int i = 0; i < syms->count; i++)
		LASSERT_TYPE(args, "def", i+1, syms->cell[i]->type, LVAL_SYM);

	LASSERT_ARGS(args, "def", syms->count, args->count - 1);

	for(int i = 0; i < syms->count; i++) {
		switch(func) {
			case(VAR_DEF): lenv_def(e, syms->cell[i], args->cell[i+1]); break;
			case(VAR_PUT): lenv_put(e, syms->cell[i], args->cell[i+1]); break;
		}
	}

	lval_del(args);
	return lval_sexp();

}

lval* builtin_def(lenv* e, lval* args) {return builtin_var(e, args, VAR_DEF);}
lval* builtin_put(lenv* e, lval* args) {return builtin_var(e, args, VAR_PUT);}

lval* builtin_lambda(lenv* e, lval* args) {

	LASSERT_ARGS(args, "\\", args->count, 2);
	LASSERT_TYPE(args, "\\", 1, args->cell[0]->type, LVAL_QEXPR);
	LASSERT_TYPE(args, "\\", 2, args->cell[1]->type, LVAL_QEXPR);

	for(int i = 0; i < args->cell[0]->count; i++)
		LASSERT_TYPE(args, "\\", i+1, args->cell[0]->cell[i]->type, LVAL_SYM);

	lval* formals = lval_pop(args, 0);
	lval* body = lval_pop(args, 0);
	lval_del(args);

	return lval_lambda(formals, body);

}

//Takes >2 args and compares them so that (eq a b c d) is equivlant to (and (eq a b) (eq b c) (eq c d))
lval* builtin_eq(lenv* e, lval* args) {

	LASSERT(args, (args->count < 2), "Function \"eq\" got wrong number of args: got %i, expected at least 2", args->count);
	lval* current = lval_pop(args, 0);
	while(args->count) {  //Loop over the arguments and test them
		lval* next = lval_pop(args, 0);
		if(lval_equals(current, next) == LVAL_FALSE){  //Not all args are equal
			lval_del(next); lval_del(current); lval_del(args);
			return LVAL_FALSE;
		}
		lval_del(current);
		current = next;
	}
	lval_del(current); lval_del(args);
	return LVAL_TRUE;

}

lval* builtin_if(lenv* e, lval* args) {

	LASSERT_ARGS(args, "if", args->count, 3);
	LASSERT_TYPE(args, "if", 1, args->cell[0]->type, LVAL_BOOL);
	LASSERT_TYPE(args, "if", 2, args->cell[1]->type, LVAL_QEXPR);
	LASSERT_TYPE(args, "if", 3, args->cell[2]->type, LVAL_QEXPR);

	args->cell[1]->type = LVAL_SEXPR;
	args->cell[2]->type = LVAL_SEXPR;

	lval* result;
	if(args->cell[0] == LVAL_TRUE) result = lval_eval(e, lval_pop(args, 1));
	else result = lval_eval(e, lval_pop(args, 2));
	lval_del(args);
	return result;

}

lval* builtin_nand(lenv* e, lval* args) {

	LASSERT_ARGS(args, "not", args->count, 2);
	LASSERT_TYPE(args, "not", 1, args->cell[0]->type, LVAL_BOOL);
	LASSERT_TYPE(args, "not", 2, args->cell[1]->type, LVAL_BOOL);

	lval* x = args->cell[0];
	lval* y = args->cell[1];
	lval_del(args);
	return (x == LVAL_TRUE && y == LVAL_TRUE) ? LVAL_FALSE : LVAL_TRUE;

}

typedef enum rel {REL_LT, REL_GT, REL_GTE, REL_LTE} rel;

lval* builtin_compare(lenv* e, lval* args, rel func) {

	LASSERT_ARGS(args, func, args->count, 2);
	LASSERT_TYPE(args, func, 1, args->cell[0]->type, LVAL_NUM);
	LASSERT_TYPE(args, func, 2, args->cell[1]->type, LVAL_NUM);

	lval* result;
	switch(func) {
		#define ADD_REL(relation, op) case(relation): result = (args->cell[0]->num op args->cell[1]->num) ? LVAL_TRUE : LVAL_FALSE; break
		ADD_REL(REL_GT, >);
		ADD_REL(REL_GTE, >=);
		ADD_REL(REL_LT, <);
		ADD_REL(REL_LTE, <=);
		#undef ADD_REL
	}
	lval_del(args);
	return result;

}

lval* builtin_gt(lenv* e, lval* args) { return builtin_compare(e, args, REL_GT);  }
lval* builtin_lt(lenv* e, lval* args) { return builtin_compare(e, args, REL_LT);  }
lval* builtin_gte(lenv* e, lval* args) { return builtin_compare(e, args, REL_GTE); }
lval* builtin_lte(lenv* e, lval* args) { return builtin_compare(e, args, REL_GT); }

lval* builtin_load(lenv* e, lval* args) {

	LASSERT_ARGS(args, "load", args->count, 1);
	LASSERT_TYPE(args, "load", 1, args->cell[0]->type, LVAL_STR);

	//Load the file
	mpc_result_t result;
	int error;
	char* filename = args->cell[0]->str;
	char* stdinstr = "stdin";
	if(strcmp(filename, stdinstr) == 0)
		error = mpc_parse_pipe(filename, stdin, Lisp, &result);
	else 
		error = mpc_parse_contents(filename, Lisp, &result);
	if(error) {
		//Parse it
		//mpc_ast_print(result.output);
		lval* expr = lval_read(result.output);
		mpc_ast_delete(result.output);

		//Evaluate it
		while(expr->count) {
			lval* v = lval_eval(e, lval_pop(expr, 0));
			if(v->type == LVAL_ERR) lval_println(v);
			lval_del(v);
		}

		//Cleanup
		lval_del(expr);
		lval_del(args);

		return lval_sexp();
	} else {
		//There was an error
		char* err_msg = mpc_err_string(result.error);
		mpc_err_delete(result.error);

		lval* err = lval_err("Could not load file: %s", err_msg);
		free(err_msg);
		lval_del(args);

		return err;
	}

}

lval* builtin_print(lenv* e, lval* args) {

	for(int i = 0; i < args->count; i++) {
		lval_print(args->cell[i]);
		putchar(' ');
	}

	putchar('\n');
	lval_del(args);
	return lval_sexp();

}

lval* builtin_err(lenv* e, lval* args) {

	LASSERT_ARGS(args, "print", args->count, 1);
	LASSERT_TYPE(args, "print", 1, args->cell[0]->type, LVAL_STR);

	lval* err = lval_err(args->cell[0]->str);
	lval_del(args);
	return err;

}

lval* builtin_exit(lenv* e, lval* args) {

	LASSERT(args, (args->count > 1), "Function \"exit\" got wrong number of args: got %i, expected 1 or less", args->count);

	int status;
	if(args->count) {
		LASSERT_TYPE(args, "exit", 1, args->cell[0]->type, LVAL_NUM);
		status = args->cell[0]->num;
	} else status = 0;

	lenv_del(e);
	mpc_cleanup(9, Number, Boolean, String, Comment, Symbol, Sexpr, Qexpr, Expr, Lisp);

	exit(status);

}

#undef LASSERT
#undef LASSERT_TYPE
#undef LASSERT_ARGS
#undef LASSERT_EMPTY

#define ADD_BUILTIN(name, operator) lval* builtin_##name(lenv* e, lval* a) { return builtin_op(e, a, #operator); }
ADD_BUILTIN(add, +)
ADD_BUILTIN(sub, -)
ADD_BUILTIN(mul, *)
ADD_BUILTIN(div, /)
ADD_BUILTIN(mod, %)
ADD_BUILTIN(pow, ^)
ADD_BUILTIN(min, min)
ADD_BUILTIN(max, max)
#undef ADD_BUILTIN

void lenv_add_builtins(lenv* e) {

	#define ADD_BUILTIN(operator, func) do{ lenv_add_builtin(e, #operator, builtin_##func); } while(false)
	ADD_BUILTIN(+,add);
	ADD_BUILTIN(-,sub);
	ADD_BUILTIN(*,mul);
	ADD_BUILTIN(/,div);
	ADD_BUILTIN(%,mod);
	ADD_BUILTIN(pow,pow);
	ADD_BUILTIN(min,min);
	ADD_BUILTIN(max,max);
	ADD_BUILTIN(list,list);
	ADD_BUILTIN(head,head);
	ADD_BUILTIN(tail,tail);
	ADD_BUILTIN(eval,eval);
	ADD_BUILTIN(join,join);
	ADD_BUILTIN(def, def);
	ADD_BUILTIN(=, put);
	ADD_BUILTIN(\\, lambda);
	ADD_BUILTIN(eq, eq);
	ADD_BUILTIN(nand, nand);
	ADD_BUILTIN(if, if);
	ADD_BUILTIN(gt, gt);
	ADD_BUILTIN(gte, gte);
	ADD_BUILTIN(lt, lt);
	ADD_BUILTIN(lte, lte);
	ADD_BUILTIN(load, load);
	ADD_BUILTIN(print, print);
	ADD_BUILTIN(exit, exit);
	ADD_BUILTIN(err, err);
	#undef ADD_BUILTIN

}

/* djb2 by Dan Bernstein
 * This and the next function retrieved from <http://www.cse.yorku.ca/~oz/hash.html> on 6/29/14
 */
unsigned long djb2(char* str){

	unsigned long hash = 5381;  //Magic starting number

	for(char c = *str; c != '\0'; c = *str++)
		hash = ((hash << 5) + hash) + c;  //Multiply by 33 and add c

	return hash;
}

//Same as above but with different magic numbers
unsigned long sdbm(char* str){

	unsigned long hash = 0;  //Another magic number

	for(char c = *str; c != '\0'; c = *str++)
		hash = ((hash << 6) + (hash << 16) - hash) + c;  //Multiply by 65599

	return hash;

}

//Helper function to sanity-check sizes
inline static int lenv_size_check(int size) {

	return !((size <= 0) || (size & (size - 1)));

}

//Create a new lenv
lenv* lenv_new(int size) {

	if(!lenv_size_check(size)) return NULL;
	lenv* e = malloc(sizeof(lenv));
	e->par = NULL;
	e->max = size;
	e->count = 0;
	e->table = malloc(sizeof(lentry) * e->max);
	for(int i = 0; i < e->max; i++) {
		e->table[i].sym = NULL;
		e->table[i].v = NULL;
	}
	return e;

}

//Delete an lenv
void lentry_del(lentry* e, int size) {

	for(int i = 0; i < size; i++) {  //Delete the lentries in e->table
		//printf("e->table[%d].v = %p\n", i, e->table[i].v);
		if(e[i].v != NULL) {
			free(e[i].sym);
			lval_del(e[i].v);
		}
	}
	free(e);
	//free(e);

}

void lenv_del(lenv* e) {

	lentry_del(e->table, e->max);
	free(e);

}

//Takes an lenv, a symbol k, and a value v
//You must free k and v
void lenv_put(lenv* e, lval* k, lval* v){

	if(((float) e->count)/((float) e->max) > 0.75f) lenv_resize(e, e->max * 2);  //Resize if the table is full

	int hash = (int) (djb2(k->str) % e->max);  //Main hash
	int dhash = (int) (9 - 2 * (sdbm(k->str) % 5));  //The jump is from 1-9 odds

	for(; (e->table[hash].sym != NULL);  hash = (hash + dhash) % e->max)  //Loop until we find an open spot
		if(strcmp(e->table[hash].sym, k->str) == 0) break; //The strings match, so we overwrite the data

	if(e->table[hash].sym != NULL){
		free(e->table[hash].sym);  //Free the old data if we're redefining the symbol
		lval_del(e->table[hash].v);
	}

	e->table[hash].sym = malloc(strlen(k->str) + 1);  //And put in the new
	strcpy(e->table[hash].sym, k->str);
	e->table[hash].v = lval_copy(v);

	e->count++;

}

lval* lenv_get(lenv* e, lval* k) {

	int hash = (int) (djb2(k->str) % e->max);  //The next few lines are the same as lenv_put()
	int dhash = (int) (9 - 2 * (sdbm(k->str) % 5));

	for(; (e->table[hash].sym != NULL);  hash = (hash + dhash) % e->max)
		if(strcmp(e->table[hash].sym, k->str) == 0) break;

	if(e->table[hash].sym != NULL) return lval_copy(e->table[hash].v);
	if(e->par)
		return lenv_get(e->par, k);
	else
		return lval_err("unbound symbol: \"%s\"", k->str);

}

void lenv_resize(lenv* e, int size) {

	if(!lenv_size_check(size)) return;
	if(size < e->max) return;  //We can't make the hashtable smaller

	int tmp_max = e->max;
	lentry* tmp = malloc(sizeof(lentry) * tmp_max);  //Stick e->table in a tmp array while we make a new table
	memcpy(tmp, e->table, sizeof(lentry) * tmp_max);
	free(e->table);

	e->table = malloc(sizeof(lentry) * size);  //Init the new table
	e->count = 0;
	e->max = size;
	for(int i = 0; i < e->max; i++) {
		e->table[i].sym = NULL;
		e->table[i].v = NULL;
	}

	for(int i = 0; i < tmp_max; i++)  //Insert the elements
		if(tmp[i].sym != NULL) {
			lval* k = lval_sym(tmp[i].sym);
			lval* v = lval_copy(tmp[i].v);
			lenv_put(e, k, v);
			lval_del(k);
			lval_del(v);
		}

	lentry_del(tmp, tmp_max);

}

lenv* lenv_copy(lenv* e) {

	lenv* x = malloc(sizeof(lenv));
	x->par = e->par;
	x->max = e->max;
	x->count = e->count;
	x->table = malloc(sizeof(lentry) * x->max);
	for(int i = 0; i < x->max; i++) {  //Copy over the elements
		if(e->table[i].sym != NULL) {
			x->table[i].sym = malloc(strlen(e->table[i].sym) + 1);
			strcpy(x->table[i].sym, e->table[i].sym);
			x->table[i].v = lval_copy(e->table[i].v);
		} else {  //If there's nothing there, there's nothing to copy
			x->table[i].sym = NULL;
			x->table[i].v = NULL;
		}
	}
	return x;

}

void lenv_def(lenv* e, lval* k, lval* v) {

	while(e->par) e = e->par;
	lenv_put(e, k, v);

}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {

	lval* k = lval_sym(name);
	lval* v = lval_func(func);
	lenv_put(e, k, v);
	lval_del(k);
	lval_del(v);

}

lval* lenv_equals(lenv* x, lenv* y) {

	if(x == y) return LVAL_TRUE;
	if(x == NULL || y == NULL) return LVAL_FALSE;  //Either x is NULL or y is NULL, but not both, therefore !=
	if((lenv_equals(x->par, y->par) == LVAL_FALSE)) return LVAL_FALSE;  //Check parents
	if(x->max != y->max) return LVAL_FALSE;
	if(x->count != y->count) return LVAL_FALSE;
	for(int i = 0; i < x->count; i++) {
		lentry xent = x->table[i];
		lentry yent = y->table[i];
		if(xent.sym == yent.sym) {
			//do nothing
		} else if((xent.sym == NULL) || (yent.sym == NULL)) {//Either one or the other is null but not both
			return LVAL_FALSE;
		} else if(strcmp(xent.sym, yent.sym) != 0) {
			return LVAL_FALSE;
		}
		//sym is equal literally or lexically
		if(xent.v == yent.v) {
			//do nothing
		} else if((xent.v == NULL) || (yent.v == NULL)) {
			return LVAL_FALSE;
		} else if(lval_equals(xent.v, yent.v) == LVAL_FALSE) {
			return LVAL_FALSE;
		}
	}
	//All the values are equal
	return LVAL_TRUE;

}
