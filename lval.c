#include "lval.h"

lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;

  return v;
}

lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;

  return v;
}

void lval_print(lval v) {
  switch (v.type) {
    case LVAL_NUM:
      printf("%li", v.num);
      break;

    case LVAL_ERR:
      if (v.err == LERR_DIV_ZERO) {
        printf("Error: Division by zero!");
      }

      if (v.err == LERR_BAD_OP) {
        printf("Error: Invalid operator!");
      }

      if (v.err == LERR_BAD_NUM) {
        printf("Error: Invalid number!");
      }
      break;
  }
}

void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}

lval lval_eval_op(lval x, char* op, lval y) {
  if (x.type == LVAL_ERR) return x;
  if (y.type == LVAL_ERR) return y;

  if (strcmp(op, "+") == 0) return lval_num(x.num + y.num);
  if (strcmp(op, "-") == 0) return lval_num(x.num - y.num);
  if (strcmp(op, "*") == 0) return lval_num(x.num * y.num);
  if (strcmp(op, "/") == 0) {
    if (y.num == 0) return lval_err(LERR_DIV_ZERO);

    return lval_num(x.num / y.num);
  }

  return lval_err(LERR_BAD_OP);
}

lval lval_eval(mpc_ast_t* t) {
  // If number, return directly
  if (strstr(t->tag, "number")) {
    // `strtol` uses a global variable, `errno`, for some reason
    errno = 0;
    long x = strtol(t->contents, NULL, 10);

    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  // The operator is always the second child, after '('
  char* op = t->children[1]->contents;

  // Store the third child in `x`
  lval x = lval_eval(t->children[2]);

  // Iterate over the remaining children and combine
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = lval_eval_op(x, op, lval_eval(t->children[i]));
    i++;
  }

  return x;
}
