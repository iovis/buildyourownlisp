#pragma once

#include "mpc.h"

enum {
  LVAL_NUM,
  LVAL_ERR,
};

enum {
  LERR_DIV_ZERO,
  LERR_BAD_OP,
  LERR_BAD_NUM,
};

typedef struct {
  int type;
  long num;
  int err;
} lval;

lval lval_num(long x);
lval lval_err(int x);
void lval_print(lval v);
void lval_println(lval v);
lval lval_eval_op(lval x, char* op, lval y);
lval lval_eval(mpc_ast_t* t);
