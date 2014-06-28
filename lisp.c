#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

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
	symbol		: '+' | '-' | '*' | '/' | '%' | '^' |	\
				\"min\" | \"max\" | \"list\" | \"head\" | \
				\"tail\" | \"join\" | \"eval\" ;		\
	qexpr		: '{' <expr>* '}' ;						\
	sexpr		: '(' <expr>* ')' ;						\
	expr		: <number> | <symbol> | <sexpr> | <qexpr>;	\
	lisp		: /^/ <expr>* /$/ ;						\
	", Number, Symbol, Qexpr, Sexpr, Expr, Lisp);

	//Version and exit info
	puts("Lisp Version 0.0.4");
	puts("Ctrl-C to exit\n");

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

		lval* tree = lval_eval(lval_read(r.output));
		lval_println(tree);
		lval_del(tree);
		mpc_ast_delete(r.output);

		free(input);

	}

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
lval* lval_err(char* message){

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(message) + 1);
	strcpy(v->err, message);
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

void lval_del(lval* v){

	switch(v->type){
		case(LVAL_NUM): break;
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
lval* lval_eval_sexpr(lval* v){

	//Evaluate children
	for(int i = 0; i < v->count; i++){
		v->cell[i] = lval_eval(v->cell[i]);
		if(v->cell[i]->type == LVAL_ERR) return lval_take(v, i);
	}

	//Deal with empty/1 value sexprs
	if(v->count == 0) return v;
	if(v->count == 1) return lval_take(v, 0);

	//Make sure we have a symbol
	lval* symbol = lval_pop(v, 0);
	if(symbol->type != LVAL_SYM){
		lval_del(symbol);
		lval_del(v);
		return lval_err("Sexpr does not start with a symbol");
	}

	lval* result = builtin(v, symbol->sym);
	lval_del(symbol);  //builtin_op deletes v
	return result;

}

lval* lval_eval(lval* v) {

	//eval sexprs
	if(v->type == LVAL_SEXPR) return lval_eval_sexpr(v);
	//And everything else
	return v;
}

lval* builtin_op(lval* args, char* op){

	for(int i = 0; i < args->count; i++){
		if(args->cell[i]->type != LVAL_NUM){
			lval_del(args);  //Also deletes op
			return lval_err("Arguments must be numbers");
		}
	}

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

//A helper assertion function
#define LASSERT(args, cond, error) do { if(cond) { lval_del(args); return lval_err(error); } } while(false)

//Return the first element in a list
lval* builtin_head(lval* args) {

	LASSERT(args, (args->count != 1), "Function \"head\" passed too many arguments");
	LASSERT(args, (args->cell[0]->type != LVAL_QEXPR), "Function \"head\" passed wrong type");
	LASSERT(args, (args->cell[0]->count == 0), "Function \"head\" passed {}");

	lval* v;
	for(v = lval_take(args, 0);
		v->count > 1;
		lval_del(lval_pop(v, 1)));  //Delete everything until we have 1 argument left

	return v;

}

//Return the last elements of the list
lval* builtin_tail(lval* args) {

	LASSERT(args, (args->count != 1), "Function \"tail\" passed too many arguments");
	LASSERT(args, (args->cell[0]->type != LVAL_QEXPR), "Function \"tail\" passed wrong type");
	LASSERT(args, (args->cell[0]->count == 0), "Function \"tail\" passed {}");

	lval* v = lval_take(args, 0);
	lval_del(lval_pop(v, 0));
	return v;

}

//"Convert" an sexpr to a qexpr
lval* builtin_list(lval* args) {

	args->type = LVAL_QEXPR;
	return args;

}

//Evaluate a qexpr
lval* builtin_eval(lval* args) {

	LASSERT(args, (args->count != 1), "Function \"eval\" passed too many arguments");
	LASSERT(args, (args->cell[0]->type != LVAL_QEXPR), "Function \"eval\" passed wrong type");

	lval* v = lval_take(args, 0);
	v->type = LVAL_SEXPR;
	return lval_eval(v);

}

lval* builtin_join(lval* args) {

	for(int i = 0; i < args->count; i++) {
		LASSERT(args, (args->cell[i]->type == LVAL_QEXPR), "Function \"join\" passed incorrect type");
	}

	lval* x;
	for(x = lval_pop(args, 0); args->count; lval_join(x, lval_pop(args, 0)));  //Join the arguments together

	lval_del(args);
	return x;

}
#undef LASSERT

lval* builtin(lval* args, char* func_name) {

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

	if(strstr("+-/*%", func_name)) return builtin_op(args, func_name);

	lval_del(args);
	return lval_err("Unknown function");

}