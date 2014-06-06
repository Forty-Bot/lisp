#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "lisp.h"

#include <editline/readline.h>
#include "mpc.h"

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
	puts("Lisp Version 0.0.1	");
	puts("Ctrl-C to exit\n");

	while(1) {

		//Read
		char* input = readline("lisp> ");
		add_history(input);

		if(input == NULL) break;  //Check for a null input

		mpc_result_t r;
		if(!(mpc_parse("<stdin>", input, Lisp, &r))){
			//Failure
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
			free(input);
			continue;
		}

		long result = eval(r.output);
		printf("%li\n", result);
		mpc_ast_delete(r.output);

		free(input);

	}

	mpc_cleanup(4, Number, Operator, Expr, Lisp);
	return 0;

}

long eval(mpc_ast_t* t){

		//Return numbers
		if(strstr(t->tag, "number")) return atoi(t->contents);

		//Get the operator
		char* op = t->children[1]->contents;

		//Evaluate the first expr
		long result = eval(t->children[2]);

		int i;
		//Loop until we hit a )
		for(i = 3; strstr(t->children[i]->tag, "expr"); i++){

			result = eval_op(op, result, eval(t->children[i]));

		}

		return result;
}

long eval_op(char* op, long a, long b){

	//This only works with operators in C, but it does cut down on the repetition
	#define ADD_OP(operator) if(strcmp(op, #operator) == 0) return a operator b;

	ADD_OP(+)
	ADD_OP(-)
	ADD_OP(*)
	ADD_OP(/)
	ADD_OP(%)

	#undef ADD_OP

	//Function should take two longs as arguments
	#define ADD_OP(operator, function) if(strcmp(op, #operator) == 0) return function(a, b);

	ADD_OP(^, pow)
	ADD_OP(min, fmin)
	ADD_OP(max, fmax)

	#undef ADD_OP

	return 0;

}