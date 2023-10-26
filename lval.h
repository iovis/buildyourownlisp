#pragma once

#include "mpc.h"

enum {
  LVAL_ERR,
  LVAL_NUM,
  LVAL_SYM,
  LVAL_SEXPR,
};

typedef struct lval {
  int type;
  long num;
  char* err;
  char* sym;
  // Count and pointer to a list of `lval`
  int count;
  struct lval** cell;
} lval;

lval* lval_num(long x);
lval* lval_err(char* message);
lval* lval_sym(char* sym);
lval* lval_sexpr(void);

static void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);

static lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
static lval* lval_add(lval* v, lval* x);

static lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v);
lval* lval_builtin_op(lval* a, char* op);

static lval* lval_pop(lval* v, int i);
static lval* lval_take(lval* v, int i);

void lval_del(lval* v);
