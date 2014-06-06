#ifndef LISP_H
#define LISP_H

#include "mpc.h"

long eval(mpc_ast_t*);
long eval_op(char*, long, long);

#endif