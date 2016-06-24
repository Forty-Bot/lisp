/**
 * lisp-forty, a lisp interpreter
 * Copyright (C) 2014 Sean Anderson
 *
 * This file is part of lisp-forty.
 *
 * lisp-forty is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with lisp-forty.  If not, see <http://www.gnu.org/licenses/>.
 */

// Fix gcc complaining about fileno
#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "std_lisp.h"
#include "mpc.h"
#include "lisp.h"
#include "version.h"

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

#ifdef WINDOWS

#include <io.h>
#define isatty _isatty
#define fileno _fileno

#else

#include <unistd.h>

#endif // WINDOWS

mpc_parser_t* Number;
mpc_parser_t* Boolean;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Symbol;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Lisp;


lenv* init() {
	
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

	mpca_lang(MPCA_LANG_DEFAULT, "                       \
	number  : /-?[0-9]+/ ;                               \
	boolean : \"true\" | \"false\" ;                     \
	string  : /\"(\\\\.|[^\"])*\"/ ;                     \
	comment : /;[^\\r\\n]*/ ;                            \
	symbol  : /[a-zA-Z0-9_+\\-%*\\/\\\\=<>!&]+/ ;        \
	qexpr   : '{' <expr>* '}' ;                          \
	sexpr   : '(' <expr>* ')' ;                          \
	expr    : <number> | <boolean> | <string> |          \
	          <comment> | <symbol> | <sexpr> | <qexpr> ; \
	lisp    : /^/ <expr>* /$/ ;",
	Number, Boolean, String, Comment, Symbol, Qexpr, Sexpr, Expr, Lisp);

	//Init the env
	lenv* e = lenv_new(LENV_INIT);
	lenv_add_builtins(e);
	
	//Load the standard library
	lval_del(parse((char*) std_lisp, e));
		
	return e;

}

int main(int argc, char** argv){

	lenv* e = init();
	
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
	puts("lisp-forty " PROJECT_VERSION "\n\n\
Copyright (C) 2014-16 Sean Anderson\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
There is NO warranty, not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n\
Ctrl-C or (exit 0) to exit");
	//printf("%i", sizeof(lval));

	while(1) {

		//Read
		char* input = readline("lisp> ");
		add_history(input);

		lval* tree = parse(input, e);
		lval_println(tree);
		lval_del(tree);
		free(input);
	}

	lenv_del(e);

	mpc_cleanup(9, Number, Boolean, String, Comment, Symbol, Sexpr, Qexpr, Expr, Lisp);
	return 0;

}

lval* parse(char* input, lenv* e) {
	
	if(input == NULL) return lval_sexp();  //Check for a null input

	mpc_result_t r;
	if(!(mpc_parse("<stdin>", input, Lisp, &r))){
		//Failure to parse the input
		lval* err = lval_err(mpc_err_string(r.error));
		mpc_err_delete(r.error);
		return err;
	}
	

	lval* tree = lval_eval(e, lval_read(r.output));
	mpc_ast_delete(r.output);
	return tree;
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
		case(LVAL_FUNC):
			return "Function";
		case(LVAL_NUM):
			return "Number";
		case(LVAL_BOOL):
			return "Boolean";
		case(LVAL_STR):
			return "String";
		case(LVAL_ERR):
			return "Error";
		case(LVAL_SYM):
			return "Symbol";
		case(LVAL_SEXPR):
			return "S-Expression";
		case(LVAL_QEXPR):
			return "Q-Expression";
		default:
			return "Not an LVAL!";
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

char* rel_name(rel func) {
	switch(func) {
		case(REL_GT):
			return "gt";
		case(REL_GTE):
			return "gte";
		case(REL_LT):
			return "lt";
		case(REL_LTE):
			return "lte";
		default:
			return "Not a comparison function!";
	}
}

lval* builtin_compare(lenv* e, lval* args, rel func) {

	LASSERT_ARGS(args, rel_name(func), args->count, 2);
	LASSERT_TYPE(args, rel_name(func), 1, args->cell[0]->type, LVAL_NUM);
	LASSERT_TYPE(args, rel_name(func), 2, args->cell[1]->type, LVAL_NUM);

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
	if(strcmp(filename, "stdin") == 0)
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
