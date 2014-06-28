#ifndef LISP_H
#define LISP_H

#include "mpc.h"

typedef struct lval {
	enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR} type;

	long num;

	char* err;

	char* sym;

	int count;
	struct lval** cell;
} lval;

//enum { LERR_DIV_0, LERR_BAD_OP, LERR_BAD_NUM};

lval* lval_num(long);
lval* lval_err(char*);
lval* lval_sym(char*);
lval* lval_sexp();

void lval_del(lval*);
lval* lval_append(lval*, lval*);
lval* lval_join(lval*, lval*);
lval* lval_take(lval*, int);
lval* lval_pop(lval*, int);

void lval_print(lval*);
void lval_expr_print(lval*, char, char);
void lval_println(lval*);

lval* lval_read(mpc_ast_t*);
lval* lval_read_num(mpc_ast_t*);

lval* lval_eval(lval*);
lval* lval_eval_sexpr(lval*);

lval* builtin_head(lval*);
lval* builtin_tail(lval*);
lval* builtin_list(lval*);
lval* builtin_eval(lval*);
lval* builtin_join(lval*);
lval* builtin_op(lval*, char*);
lval* builtin(lval*, char*);

//lval eval(mpc_ast_t*);
//lval eval_op(char*, lval, lval);
//lval divide(long, long);

#endif