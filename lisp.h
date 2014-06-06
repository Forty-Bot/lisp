#ifndef LISP_H
#define LISP_H

#include "mpc.h"

//Holds either the error or result
typedef union  {
	int err;
	long num;
} result_t;

typedef struct {
	int type;
	result_t r;
} lval;

enum { LVAL_NUM, LVAL_ERR};

enum { LERR_DIV_0, LERR_BAD_OP, LERR_BAD_NUM};

lval lval_err(int);
lval lval_num(long);
void lval_print(lval);
void lval_println(lval);

lval eval(mpc_ast_t*);
lval eval_op(char*, lval, lval);
lval divide(long, long);

#endif