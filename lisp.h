#ifndef LISP_H
#define LISP_H

#include "mpc.h"

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval{
	enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUNC} type;

	long num;

	char* err;

	char* sym;

	lbuiltin func;

	int count;
	struct lval** cell;
};

typedef struct lentry{
	char* sym;
	lval* v;
} lentry;

//lenvs are a doubly hashed hashtable
struct lenv{
	int max;
	int count;
	lentry* table;
};

/* Initial size of hash table
 * The hash table size should always be a factor of 2
 * This means our double hash needs to be odd, but we can make that work
 */
#define LENV_INIT 32

//enum { LERR_DIV_0, LERR_BAD_OP, LERR_BAD_NUM};

#define LVAL_ERR_MAX 512

lval* lval_num(long);
lval* lval_err(char*, ...);
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

lval* lval_eval(lenv*, lval*);
lval* lval_eval_sexpr(lenv*, lval*);

char* ltype_name(int);

lval* builtin_head(lenv*, lval*);
lval* builtin_tail(lenv*, lval*);
lval* builtin_list(lenv*, lval*);
lval* builtin_eval(lenv*, lval*);
lval* builtin_join(lenv*, lval*);
lval* builtin_op(lenv*, lval*, char*);
lval* builtin(lval*, char*);

lenv* lenv_new(int);
void lenv_del(lenv*);
void lenv_put(lenv*, lval*, lval*);
lval* lenv_get(lenv*, lval*);
void lenv_resize(lenv*, int);
void lenv_add_builtin(lenv*, char*, lbuiltin);
void lenv_add_builtins(lenv*);

//lval eval(mpc_ast_t*);
//lval eval_op(char*, lval, lval);
//lval divide(long, long);

#endif