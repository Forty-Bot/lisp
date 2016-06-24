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
#include <stdarg.h>

#include "mpc.h"

#include "lisp.h"

//Create an lval from a given number
lval* lval_num(const long num){

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
	v->str = malloc(LVAL_ERR_MAX);
	vsnprintf(v->str, LVAL_ERR_MAX - 1, fmt, va);
	v->str = realloc(v->str, strlen(v->str) + 1);  //Free excess space in the string
	va_end(va);
	return v;

}

lval* lval_str(char* str){

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_STR;
	v->str = malloc(strlen(str) + 1);
	strcpy(v->str, str);
	return v;

}

//A sym lval from the message
lval* lval_sym(char* message){

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->str = malloc(strlen(message) + 1);
	strcpy(v->str, message);
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
	v->builtin = func;
	return v;

}

lval* lval_lambda(lval* formals, lval* body) {

	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUNC;
	v->builtin = NULL;
	v->env  = lenv_new(LENV_LOCAL_INIT);
	v->formals = formals;
	v->body = body;
	return v;

}

static lval L_TRUE = {LVAL_BOOL, {true}};
static lval L_FALSE = {LVAL_BOOL, {false}};
lval* LVAL_TRUE = &L_TRUE;
lval* LVAL_FALSE = &L_FALSE;

//Booleans are immutable, so we only return pointers to LVAL_TRUE or LVAL_FALSE
lval* lval_bool(int boolean) {

	if(boolean)
		return LVAL_TRUE;
	else
		return LVAL_FALSE;

}

void lval_del(lval* v){

	switch(v->type){
		case(LVAL_NUM): break;
		case(LVAL_BOOL): return;  //Can't free an immutable
		case(LVAL_FUNC): if(!v->builtin) {
			lenv_del(v->env);
			lval_del(v->formals);
			lval_del(v->body);
		} break;
		case(LVAL_STR): free(v->str); break;
		case(LVAL_ERR): free(v->str); break;
		case(LVAL_SYM): free(v->str); break;
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

//Tests to see if two lvals are equal (identical)
lval* lval_equals(lval* x, lval* y) {

	if(x == y) return LVAL_TRUE;  //This takes care of booleans, since immutibility
	if(x->type != y->type) return LVAL_FALSE;  //TODO: should we error on this?

	switch(x->type) {
		case(LVAL_BOOL):
			break;
		case(LVAL_NUM): 
			if(x->num == y->num) return LVAL_TRUE; 
			break;
		case(LVAL_FUNC): if(x->builtin) {
			if(x->builtin == y->builtin) return LVAL_TRUE;
				break;
			} else {
				if((lenv_equals(x->env, y->env) == LVAL_TRUE) &&
				   (lval_equals(x->formals, y->formals) == LVAL_TRUE) &&
				   (lval_equals(x->body, y->body) == LVAL_TRUE))
					return LVAL_TRUE;
				break;
			}
		case(LVAL_ERR):
		case(LVAL_SYM):
		case(LVAL_STR):
			if(strcmp(x->str, y->str) == 0) return LVAL_TRUE;
			break;
		case(LVAL_QEXPR):
		case(LVAL_SEXPR):
			if(x->count != y->count) break;
			for(int i = 0; i < x->count; i++) {
				if(lval_equals(x->cell[i], y->cell[i]) == LVAL_FALSE) break;
			}
			return LVAL_TRUE;  //Everything equals
	}
	return LVAL_FALSE;

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
		case(LVAL_BOOL): free(x); return v;
		case(LVAL_FUNC): if(v->builtin) {
			x->builtin = v->builtin;
		} else {
			x->builtin = NULL;
			x->env = lenv_copy(v->env);
			x->formals = lval_copy(v->formals);
			x->body = lval_copy(v->body);
		} break;
		case(LVAL_ERR): x->str = malloc(strlen(v->str) + 1); strcpy(x->str, v->str); break;
		case(LVAL_SYM): x->str = malloc(strlen(v->str) + 1); strcpy(x->str, v->str); break;
		case(LVAL_STR): x->str = malloc(strlen(v->str) + 1); strcpy(x->str, v->str); break;
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

void lval_print(lval* v){

	switch(v->type){
		case(LVAL_BOOL):
			if(v == LVAL_TRUE) {
				printf("true");
			} else {
				printf("false"); 
			}
			break;
		case(LVAL_NUM):
			printf("%li", v->num);
			break;
		case(LVAL_ERR):
			printf("Error: %s", v->str);
			break;
		case(LVAL_SYM):
			printf("%s", v->str);
			break;
		case(LVAL_STR):
			lval_str_print(v);
			break;
		case(LVAL_FUNC):
			if(v->builtin) {
				printf("<builtin>");
			} else {
				printf("(\\ "); lval_print(v->formals);
				putchar(' ');
				lval_print(v->body); putchar(')');
			}
			break;
		case(LVAL_SEXPR):
			lval_expr_print(v, '(', ')');
			break;
		case(LVAL_QEXPR):
			lval_expr_print(v, '{', '}');
			break;
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

void lval_str_print(lval* v){

	char* escaped = malloc(strlen(v->str) + 1);
	strcpy(escaped, v->str);

	escaped = mpcf_escape(escaped);
	printf("\"%s\"", escaped);
	free(escaped);

}

void lval_println(lval* v){
	lval_print(v); putchar('\n');
}
