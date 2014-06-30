#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#include <editline/readline.h>

#include "mpc.h"
#include "lisp.h"

int main(int argc, char** argv){

	//Init the parser
	mpc_parser_t* Number	= mpc_new("number");
	mpc_parser_t* Symbol	= mpc_new("symbol");
	mpc_parser_t* Sexpr		= mpc_new("sexpr");
	mpc_parser_t* Qexpr		= mpc_new("qexpr");
	mpc_parser_t* Expr		= mpc_new("expr");
	mpc_parser_t* Lisp		= mpc_new("lisp");

	mpca_lang(MPCA_LANG_DEFAULT,
	"\
	number		: /-?[0-9]+/ ;							\
	symbol		: /[a-zA-Z0-+\\-%*\\/\\\\=<>!&]+/ ;		\
	qexpr		: '{' <expr>* '}' ;						\
	sexpr		: '(' <expr>* ')' ;						\
	expr		: <number> | <symbol> | <sexpr> | <qexpr>;	\
	lisp		: /^/ <expr>* /$/ ;						\
	", Number, Symbol, Qexpr, Sexpr, Expr, Lisp);

	//Version and exit info
	puts("Lisp Version 0.0.4");
	puts("Ctrl-C to exit\n");

	lenv* e = lenv_new(LENV_INIT);
	lenv_add_builtins(e);

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

	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lisp);
	return 0;

}

/*lval eval(mpc_ast_t* t){

		//Return numbers
		if(strstr(t->tag, "number")) {

			errno = 0;
			int number = strtol(t->contents, NULL, 10);  //Base 10
			return errno != ERANGE ? lval_num(number) : lval_err(LERR_BAD_NUM);

		}
		//Get the operator
		char* op = t->children[1]->contents;

		//Evaluate the first expr
		lval result = eval(t->children[2]);

		//Loop until we hit a )
		for(int i = 3; strstr(t->children[i]->tag, "expr"); i++){

			result = eval_op(op, result, eval(t->children[i]));

		}

		return result;
}*/

/*lval eval_op(char* op, lval a, lval b){

	//If there is an error, pass it up the chain
	if(a.type == LVAL_ERR) return a;
	if(b.type == LVAL_ERR) return b;

	//This only works with operators in C, but it does cut down on the repetition
	#define ADD_OP(operator) if(strcmp(op, #operator) == 0) return lval_num(a.r.num operator b.r.num);

	ADD_OP(+)
	ADD_OP(-)
	ADD_OP(*)
	ADD_OP(%)

	#undef ADD_OP

	//These functions should take two longs as arguments
	#define ADD_OP(operator, function) if(strcmp(op, #operator) == 0) return lval_num(function(a.r.num, b.r.num));

	ADD_OP(^, pow)
	ADD_OP(min, fmin)
	ADD_OP(max, fmax)

	#undef ADD_OP

	//These functions return lvals
	#define ADD_OP(operator, function) if(strcmp(op, #operator) == 0) return function(a.r.num, b.r.num);

	ADD_OP(/, divide)

	#undef ADD_OP

	return lval_err(LERR_BAD_OP);

}*/

/*lval divide(long a, long b){

	return b == 0 ? lval_err("Division by zero") : lval_num(a / b);

}*/

//Create an lval from a given number
lval* lval_num(long num){

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
	v->err = malloc(LVAL_ERR_MAX);
	vsnprintf(v->err, LVAL_ERR_MAX - 1, fmt, va);
	v->err = realloc(v->err, strlen(v->err) + 1);  //Free excess space in the string
	va_end(va);
	return v;

}

//A sym lval from the message
lval* lval_sym(char* message){

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(message) + 1);
	strcpy(v->sym, message);
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
	v->func = func;
	return v;

}

void lval_del(lval* v){

	switch(v->type){
		case(LVAL_NUM):
		case(LVAL_FUNC): break;
		case(LVAL_ERR): free(v->err); break;
		case(LVAL_SYM): free(v->sym); break;
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
		case(LVAL_FUNC): x->func = v->func; break;
		case(LVAL_ERR): x->err = malloc(strlen(v->err) + 1); strcpy(x->err, v->err); break;
		case(LVAL_SYM): x->sym = malloc(strlen(v->sym) + 1); strcpy(x->sym, v->sym); break;
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

//Print an lval to stddout
/*void lval_print(lval v){

		switch(v.type){
			case(LVAL_NUM):
				printf("%li", v.r.num); break;

			case(LVAL_ERR):
				#define PRINT_E(error, message) if (v.r.err == (error)) { printf(message); }
				PRINT_E(LERR_BAD_NUM, "Error: Invalid Number")
				else PRINT_E(LERR_BAD_OP, "Error: Invalid Operator")
				else PRINT_E(LERR_DIV_0, "Error: Divide by Zero")
				#undef PRINT_E
			break;
		}
}*/

void lval_print(lval* v){

	switch(v->type){
		case(LVAL_NUM): printf("%li", v->num); break;
		case(LVAL_ERR): printf("Error: %s", v->err); break;
		case(LVAL_SYM): printf("%s", v->sym); break;
		case(LVAL_FUNC): printf("<function>"); break;
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

void lval_println(lval* v){
	lval_print(v); putchar('\n');
}

//Read a number from an abstract syntax tree
lval* lval_read_num(mpc_ast_t* t){

	errno = 0;
	long num = strtol(t->contents, NULL, 10);  //Base 10
	return errno != ERANGE ? lval_num(num) : lval_err("invalid number");

}

lval* lval_read(mpc_ast_t* t){

	//Return if it's a symbol or number
	if(strstr(t->tag, "number")) return lval_read_num(t);
	if(strstr(t->tag, "symbol")) return lval_sym(t->contents);

	//It should be an sexpr or the root
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
	if(v->count == 1) return lval_take(v, 0);

	//Make sure we have a symbol
	lval* symbol = lval_pop(v, 0);
	if(symbol->type != LVAL_FUNC){
		lval_del(symbol);
		lval_del(v);
		return lval_err("Sexpr does not start with a function");
	}

	lval* result = symbol->func(e, v);
	lval_del(symbol);  //builtin_op deletes v
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

char* ltype_name(int type){

	switch(type) {
		case(LVAL_FUNC): return "Function";
		case(LVAL_NUM): return "Number";
		case(LVAL_ERR): return "Error";
		case(LVAL_SYM): return "Symbol";
		case(LVAL_SEXPR): return "S-Expression";
		case(LVAL_QEXPR): return "Q-Expression";
		default: return "Unknown";
	}

}

//A helper assertion function
#define LASSERT(args, cond, fmt, ...) do { if(cond) { lval* err = lval_err(fmt, __VA_ARGS__); lval_del(args); return err;} } while(false)
//Check if we have the right type (arg is the number of the argument)
#define LASSERT_TYPE(args, func, arg, type, expected) do { LASSERT(args, (type != expected), "Function \"%s\" passed incorrect type for argument %i: got %s, expected %s", func, arg, ltype_name(type), ltype_name(expected)); } while(false)
//Check if we got the right number of args
#define LASSERT_ARGS(args, func, num_args, expected) do { LASSERT(args, (num_args != expected), "Function \"%s\" passed wrong number of args: got %i, expected %i", func, num_args, expected); } while(false)
//Check if the list is empty
#define LASSERT_EMPTY(args, func, list) do { LASSERT(args, (list->count == 0), "Function \"%s\" passed empty %s", func, ltype_name(list->type)); } while(false)

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

lval* builtin_def(lenv* e, lval* args) {

	LASSERT_TYPE(args, "def", 0, args->cell[0]->type, LVAL_QEXPR);

	lval* syms = args->cell[0];

	for(int i = 0; i < syms->count; i++)
		LASSERT_TYPE(args, "def", i+1, syms->cell[i]->type, LVAL_SYM);

	LASSERT_ARGS(args, "def", syms->count, args->count - 1);

	for(int i = 0; i < syms->count; i++)
		lenv_put(e, syms->cell[i], args->cell[i+1]);

	lval_del(args);
	return lval_sexp();

}
#undef LASSERT
#undef LASSERT_TYPE

#define ADD_BUILTIN(name, operator) lval* builtin_##name(lenv* e, lval* a) { return builtin_op(e, a, #operator); }
ADD_BUILTIN(add, +)
ADD_BUILTIN(sub, -)
ADD_BUILTIN(mul, *)
ADD_BUILTIN(div, /)
ADD_BUILTIN(mod, %)
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
	ADD_BUILTIN(min,min);
	ADD_BUILTIN(max,max);
	ADD_BUILTIN(list,list);
	ADD_BUILTIN(head,head);
	ADD_BUILTIN(tail,tail);
	ADD_BUILTIN(eval,eval);
	ADD_BUILTIN(join,join);
	ADD_BUILTIN(def, def);
	#undef ADD_BUILTIN

}

/*lval* builtin(lval* args, char* func_name) {

	//Helper macro for builtin_funcs
	#define ADD_FUNC(function) do{ if(strcmp(#function, func_name) == 0) return builtin_##function(args); } while(false)
	ADD_FUNC(list);
	ADD_FUNC(head);
	ADD_FUNC(tail);
	ADD_FUNC(join);
	ADD_FUNC(eval);
	#undef ADD_FUNC

	//Helper macro for math.h functions (implemented in builtin_op)
	#define ADD_FUNC(operator) do{ if(strcmp(#operator, func_name) == 0) return builtin_op(args, func_name); } while(false)
	ADD_FUNC(^);
	ADD_FUNC(min);
	ADD_FUNC(max);
	#undef ADD_FUNC

	if(strstr("+-*%/", func_name)) return builtin_op(args, func_name);

	lval_del(args);
	return lval_err("Unknown function");

}*/

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

//Create a new lenv
lenv* lenv_new(int size) {

	lenv* e = malloc(sizeof(lenv));
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
void lenv_del(lenv* e) {

	for(int i = 0; i < e->max; i++) {  //Delete the lentries in e->table
		free(e->table[i].sym);
		lval_del(e->table[i].v);
	}
	free(e->table);
	free(e);

}

//Takes an lenv, a symbol k, and a value v
void lenv_put(lenv* e, lval* k, lval* v){

	if(((float) e->count)/((float) e->max) > 0.75f) e = lenv_resize(e, e->max * 2);  //Resize if the table is full

	int hash = (int) (djb2(k->sym) % e->max);  //Main hash
	int dhash = (int) (9 - 2 * (sdbm(k->sym) % 5));  //The jump is from 1-9 odds

	for(; (e->table[hash].sym != NULL);  hash = (hash + dhash) % e->max)  //Loop until we find an open spot
		if(strcmp(e->table[hash].sym, k->sym) == 0) break; //The strings match, so we overwrite the data

	if(e->table[hash].sym != NULL){
		free(e->table[hash].sym);  //Free the old data if we're redefining the symbol
		lval_del(e->table[hash].v);
	}

	e->table[hash].sym = malloc(strlen(k->sym));  //And put in the new
	strcpy(e->table[hash].sym, k->sym);
	e->table[hash].v = lval_copy(v);

	e->count++;

}

lval* lenv_get(lenv* e, lval* k) {

	int hash = (int) (djb2(k->sym) % e->max);  //The next few lines are the same as lenv_put()
	int dhash = (int) (9 - 2 * (sdbm(k->sym) % 5));

	for(; (e->table[hash].sym != NULL);  hash = (hash + dhash) % e->max)
		if(strcmp(e->table[hash].sym, k->sym) == 0) break;

	if(e->table[hash].sym == NULL) return lval_err("unbound symbol");
	else return lval_copy(e->table[hash].v);
}

lenv* lenv_resize(lenv* e, int size) {

	lenv* x = lenv_new(size);

	for(int i = 0; i < e->max; i++)
		if(e->table[i].sym != NULL) lenv_put(x, lval_sym(e->table[i].sym), e->table[i].v);

	lenv_del(e);
	return x;

}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {

	lval* k = lval_sym(name);
	lval* v = lval_func(func);
	lenv_put(e, k, v);
	lval_del(k);
	lval_del(v);

}