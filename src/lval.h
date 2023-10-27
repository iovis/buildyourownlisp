#pragma once

#include "mpc.h"

enum {
  LVAL_ERR,
  LVAL_NUM,
  LVAL_SYM,
  LVAL_SEXPR,
  LVAL_QEXPR,
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
lval* lval_qexpr(void);

void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);

lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
lval* lval_add(lval* v, lval* x);

lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v);
lval* lval_builtin(lval* a, char* func);
lval* lval_builtin_op(lval* a, char* op);
lval* lval_builtin_head(lval* a);
lval* lval_builtin_tail(lval* a);
lval* lval_builtin_list(lval* a);
lval* lval_builtin_eval(lval* a);
lval* lval_builtin_join(lval* a);
lval* lval_builtin_len(lval* a);

lval* lval_join(lval* x, lval* y);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);

void lval_del(lval* v);