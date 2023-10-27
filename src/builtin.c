#include "builtin.h"

#include "lval.h"

#define LASSERT(args, cond, err) \
  if (!(cond)) {                 \
    lval_del(args);              \
    return lval_err(err);        \
  }

lval* builtin_op(lenv* e, lval* a, char* op) {
  // Ensure all arguments are numbers
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number");
    }
  }

  lval* x = lval_pop(a, 0);

  // If only one element and `-`, perform unary operation
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  while (a->count > 0) {
    // Pop the next element
    lval* y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) x->num += y->num;
    if (strcmp(op, "-") == 0) x->num -= y->num;
    if (strcmp(op, "*") == 0) x->num *= y->num;
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);

        x = lval_err("Division by zero!");
      }

      x->num /= y->num;
    }

    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval* builtin_add(lenv* e, lval* a) { return builtin_op(e, a, "+"); }
lval* builtin_sub(lenv* e, lval* a) { return builtin_op(e, a, "-"); }
lval* builtin_mul(lenv* e, lval* a) { return builtin_op(e, a, "*"); }
lval* builtin_div(lenv* e, lval* a) { return builtin_op(e, a, "/"); }

lval* builtin_head(lenv* e, lval* a) {
  // Error conditions
  LASSERT(a, a->count == 1, "Function 'head' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'head' passed incorrect types!");
  LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed {}!");

  // Otherwise, take first argument
  lval* v = lval_take(a, 0);

  // Cleanup
  while (v->count > 1) lval_del(lval_pop(v, 1));

  return v;
}

lval* builtin_tail(lenv* e, lval* a) {
  // Error conditions
  LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'tail' passed incorrect types!");
  LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed {}!");

  // Otherwise, take first argument
  lval* v = lval_take(a, 0);

  // Delete first element and return
  lval_del(lval_pop(v, 0));

  return v;
}

lval* builtin_list(lenv* e, lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lenv* e, lval* a) {
  // Error conditions
  LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'tail' passed incorrect types!");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;

  return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
            "Function 'join' passed incorrect type.");
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);

  return x;
}

lval* builtin_len(lenv* e, lval* a) {
  LASSERT(a, a->count == 1, "Function 'len' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'len' passed incorrect type.");

  lval* x = lval_take(a, 0);
  lval* len = lval_num(x->count);

  lval_del(x);

  return len;
}

lval* builtin_def(lenv* e, lval* a) {
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
      "Function 'def' passed incorrect types!");

  // First argument is a symbol list
  lval* symbols = a->cell[0];

  // Validate they're all symbols
  for (int i = 0; i < symbols->count; i++) {
    LASSERT(a, symbols->cell[i]->type == LVAL_SYM,
        "Function 'def' cannot define non-symbol");
  }

  LASSERT(a, symbols->count == a->count - 1,
      "Function 'def' cannot define incorrect "
      "number of values to symbols");

  // Assign copies of values to symbols
  for (int i = 0; i < symbols->count; i++) {
    lenv_put(e, symbols->cell[i], a->cell[i + 1]);
  }

  lval_del(a);

  return lval_sexpr();
}

void add_builtins(lenv* e) {
  // List functions
  lenv_add_builtin(e, "def", builtin_def);
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);

  // Mathematical functions
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
}
