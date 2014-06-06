#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>

#include "mpc.h"
#include "lisp.h"

int main(int argc, char** argv){

	//Init the parser
	mpc_parser_t* Number	= mpc_new("number");
	mpc_parser_t* Operator	= mpc_new("operator");
	mpc_parser_t* Expr		= mpc_new("expr");
	mpc_parser_t* Lisp		= mpc_new("lisp");

	mpca_lang(MPCA_LANG_DEFAULT,
	"\
	number		: /-?[0-9]+/ ;								\
	operator	: '+' | '-' | '*' | '/' | '%' | '^' |		\
				\"min\" | \"max\";	\
	expr		: <number> | '(' <operator> <expr>+ ')' ;	\
	lisp		: /^/ <operator> <expr>+ /$/ ;				\
	", Number, Operator, Expr, Lisp);

	//Version and exit info
	puts("Lisp Version 0.0.1");
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

		lval result = eval(r.output);
		lval_println(result);
		mpc_ast_delete(r.output);

		free(input);

	}

	mpc_cleanup(4, Number, Operator, Expr, Lisp);
	return 0;

}

lval eval(mpc_ast_t* t){

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

		int i;
		//Loop until we hit a )
		for(i = 3; strstr(t->children[i]->tag, "expr"); i++){

			result = eval_op(op, result, eval(t->children[i]));

		}

		return result;
}

lval eval_op(char* op, lval a, lval b){

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

}

lval divide(long a, long b){

	return b == 0 ? lval_err(LERR_DIV_0) : lval_num(a / b);

}

//Create an lval from a given error
lval lval_err(int err){

	lval v;
	v.type = LVAL_ERR;
	v.r.err = err;
	return v;

}

//Create an lval from a given number
lval lval_num(long num){

	lval v;
	v.type = LVAL_NUM;
	v.r.num = num;
	return v;

}

//Print an lval to stddout
void lval_print(lval v){

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
}

void lval_println(lval v){
	lval_print(v); putchar('\n');
}