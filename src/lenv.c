/**
 * lisp-forty, a lisp interpreter
 * Copyright (C) 2014-16 Sean Anderson
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

#include "lisp.h"

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

//Helper function to sanity-check sizes
inline static int lenv_size_check(int size) {

	return !((size <= 0) || (size & (size - 1)));

}

//Create a new lenv
lenv* lenv_new(int size) {

	if(!lenv_size_check(size)) return NULL;
	lenv* e = malloc(sizeof(lenv));
	e->par = NULL;
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
void lentry_del(lentry* e, int size) {

	for(int i = 0; i < size; i++) {  //Delete the lentries in e->table
		if(e[i].v != NULL) {
			free(e[i].sym);
			lval_del(e[i].v);
		}
	}
	free(e);

}

void lenv_del(lenv* e) {

	lentry_del(e->table, e->max);
	free(e);

}

void lenv_resize(lenv* e, int size) {

	if(!lenv_size_check(size)) return;
	if(size < e->max) return;  //We can't make the hashtable smaller

	int tmp_max = e->max;
	lentry* tmp = malloc(sizeof(lentry) * tmp_max);  //Stick e->table in a tmp array while we make a new table
	memcpy(tmp, e->table, sizeof(lentry) * tmp_max);
	free(e->table);

	e->table = malloc(sizeof(lentry) * size);  //Init the new table
	e->count = 0;
	e->max = size;
	for(int i = 0; i < e->max; i++) {
		e->table[i].sym = NULL;
		e->table[i].v = NULL;
	}

	for(int i = 0; i < tmp_max; i++)  //Insert the elements
		if(tmp[i].sym != NULL) {
			lval* k = lval_sym(tmp[i].sym);
			lval* v = lval_copy(tmp[i].v);
			lenv_put(e, k, v);
			lval_del(k);
			lval_del(v);
		}

	lentry_del(tmp, tmp_max);

}

//Takes an lenv, a symbol k, and a value v
//You must free k and v
void lenv_put(lenv* e, lval* k, lval* v){

	if(((float) e->count)/((float) e->max) > 0.75f) lenv_resize(e, e->max * 2);  //Resize if the table is full

	int hash = (int) (djb2(k->str) % e->max);  //Main hash
	int dhash = (int) (9 - 2 * (sdbm(k->str) % 5));  //The jump is from 1-9 odds

	for(; (e->table[hash].sym != NULL);  hash = (hash + dhash) % e->max)  //Loop until we find an open spot
		if(strcmp(e->table[hash].sym, k->str) == 0) break; //The strings match, so we overwrite the data

	if(e->table[hash].sym != NULL){
		free(e->table[hash].sym);  //Free the old data if we're redefining the symbol
		lval_del(e->table[hash].v);
	}

	e->table[hash].sym = malloc(strlen(k->str) + 1);  //And put in the new
	strcpy(e->table[hash].sym, k->str);
	e->table[hash].v = lval_copy(v);

	e->count++;

}

lval* lenv_get(lenv* e, lval* k) {

	int hash = (int) (djb2(k->str) % e->max);  //The next few lines are the same as lenv_put()
	int dhash = (int) (9 - 2 * (sdbm(k->str) % 5));

	for(; (e->table[hash].sym != NULL);  hash = (hash + dhash) % e->max)
		if(strcmp(e->table[hash].sym, k->str) == 0) break;

	if(e->table[hash].sym != NULL) return lval_copy(e->table[hash].v);
	if(e->par)
		return lenv_get(e->par, k);
	else
		return lval_err("unbound symbol: \"%s\"", k->str);

}

lenv* lenv_copy(lenv* e) {

	lenv* x = malloc(sizeof(lenv));
	x->par = e->par;
	x->max = e->max;
	x->count = e->count;
	x->table = malloc(sizeof(lentry) * x->max);
	for(int i = 0; i < x->max; i++) {  //Copy over the elements
		if(e->table[i].sym != NULL) {
			x->table[i].sym = malloc(strlen(e->table[i].sym) + 1);
			strcpy(x->table[i].sym, e->table[i].sym);
			x->table[i].v = lval_copy(e->table[i].v);
		} else {  //If there's nothing there, there's nothing to copy
			x->table[i].sym = NULL;
			x->table[i].v = NULL;
		}
	}
	return x;

}

void lenv_def(lenv* e, lval* k, lval* v) {

	while(e->par) e = e->par;
	lenv_put(e, k, v);

}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {

	lval* k = lval_sym(name);
	lval* v = lval_func(func);
	lenv_put(e, k, v);
	lval_del(k);
	lval_del(v);

}

lval* lenv_equals(lenv* x, lenv* y) {

	if(x == y) return LVAL_TRUE;
	if(x == NULL || y == NULL) return LVAL_FALSE;  //Either x is NULL or y is NULL, but not both, therefore !=
	if((lenv_equals(x->par, y->par) == LVAL_FALSE)) return LVAL_FALSE;  //Check parents
	if(x->max != y->max) return LVAL_FALSE;
	if(x->count != y->count) return LVAL_FALSE;
	for(int i = 0; i < x->count; i++) {
		lentry xent = x->table[i];
		lentry yent = y->table[i];
		if(xent.sym == yent.sym) {
			//do nothing
		} else if((xent.sym == NULL) || (yent.sym == NULL)) {//Either one or the other is null but not both
			return LVAL_FALSE;
		} else if(strcmp(xent.sym, yent.sym) != 0) {
			return LVAL_FALSE;
		}
		//sym is equal literally or lexically
		if(xent.v == yent.v) {
			//do nothing
		} else if((xent.v == NULL) || (yent.v == NULL)) {
			return LVAL_FALSE;
		} else if(lval_equals(xent.v, yent.v) == LVAL_FALSE) {
			return LVAL_FALSE;
		}
	}
	//All the values are equal
	return LVAL_TRUE;

}
