#pragma once

#include "mpc.h"

typedef struct lval lval;
typedef struct lenv lenv;

// Function pointers for builtins: `lbuiltin`
// Ex: lval* my_builtin(lenv*, lval*);
typedef lval* (*lbuiltin)(lenv*, lval*);

enum {
  LVAL_ERR,
  LVAL_NUM,
  LVAL_SYM,
  LVAL_SEXPR,
  LVAL_QEXPR,
  LVAL_FUN,
};

struct lval {
  int type;

  long num;
  char* err;
  char* sym;
  lbuiltin fun;

  // Count and pointer to a list of `lval`
  int count;
  lval** cell;
};

// Holds variables. Contains the relationship between names (symbols) and values
struct lenv {
  int count;
  char** syms;
  lval** vals;
};

lval* lval_num(long x);
lval* lval_err(char* fmt, ...);
lval* lval_sym(char* sym);
lval* lval_sexpr(void);
lval* lval_qexpr(void);
lval* lval_fun(lbuiltin func);
char* ltype_name(int t);

void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);

lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
lval* lval_add(lval* v, lval* x);

lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_eval(lenv* e, lval* v);

lval* lval_join(lval* x, lval* y);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);

void lval_del(lval* v);

lenv* lenv_new(void);
lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* k, lval* v);
void lenv_add_builtin(lenv* e, char* name, lbuiltin func);
void lenv_del(lenv* e);
