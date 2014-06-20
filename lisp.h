#ifndef LISP_H
#define LISP_H

#include "mpc.h"

typedef struct lval {
	enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR} type;

	long num;

	char* err;

	char* sym;

	int count;
	struct lval** cell;
} lval;

enum { LERR_DIV_0, LERR_BAD_OP, LERR_BAD_NUM};

lval* lval_num(long);
lval* lval_err(char*);
lval* lval_sym(char*);
lval* lval_sexp();
void lval_del(lval*);
lval* lval_append(lval*, lval*);

void lval_print(lval*);
void lval_expr_print(lval*, char, char);
void lval_println(lval*);

lval* lval_read(mpc_ast_t*);
lval* lval_read_num(mpc_ast_t*);

lval* builtin_op(lval*, char*);
lval* lval_eval(lval*);
lval* lval_eval_sexpr(lval*);

//lval eval(mpc_ast_t*);
//lval eval_op(char*, lval, lval);
//lval divide(long, long);

#endif