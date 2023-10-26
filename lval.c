#include "lval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LASSERT(args, cond, err) \
  if (!(cond)) {                 \
    lval_del(args);              \
    return lval_err(err);        \
  }

lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));

  v->type = LVAL_NUM;
  v->num = x;

  return v;
}

lval* lval_err(char* message) {
  lval* v = malloc(sizeof(lval));

  v->type = LVAL_ERR;
  v->err = malloc(strlen(message) + 1);
  strcpy(v->err, message);

  return v;
}

lval* lval_sym(char* sym) {
  lval* v = malloc(sizeof(lval));

  v->type = LVAL_SYM;
  v->sym = malloc(strlen(sym) + 1);
  strcpy(v->sym, sym);

  return v;
}

lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));

  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;

  return v;
}

lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));

  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;

  return v;
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);

  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);

    if (i != (v->count - 1)) putchar(' ');
  }

  putchar(close);
}

void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_ERR:
      printf("Error: %s", v->err);
      break;
    case LVAL_NUM:
      printf("%li", v->num);
      break;
    case LVAL_SYM:
      printf("%s", v->sym);
      break;
    case LVAL_SEXPR:
      lval_expr_print(v, '(', ')');
      break;
    case LVAL_QEXPR:
      lval_expr_print(v, '{', '}');
      break;
  }
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

static void lval_debug(lval* v) {
  printf("[DEBUG]: ");
  lval_print(v);
  putchar('\n');
}

lval* lval_read_num(mpc_ast_t* t) {
  // `strtol` uses a global variable, `errno`, for some reason
  errno = 0;
  long x = strtol(t->contents, NULL, 10);

  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) return lval_read_num(t);
  if (strstr(t->tag, "symbol")) return lval_sym(t->contents);

  // If root (>) or sexpr then create an empty list
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) x = lval_sexpr();
  if (strstr(t->tag, "sexpr")) x = lval_sexpr();
  if (strstr(t->tag, "qexpr")) x = lval_qexpr();

  // Fill this list with any valid expressions
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) continue;
    if (strcmp(t->children[i]->contents, ")") == 0) continue;
    if (strcmp(t->children[i]->contents, "{") == 0) continue;
    if (strcmp(t->children[i]->contents, "}") == 0) continue;
    if (strcmp(t->children[i]->tag, "regex") == 0) continue;

    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count - 1] = x;

  return v;
}

lval* lval_eval_sexpr(lval* v) {
  // Eval children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  // Error checking
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) return lval_take(v, i);
  }

  // Empty expression
  if (v->count == 0) return v;

  // Single expression
  if (v->count == 1) return lval_take(v, 0);

  // More than one child
  // Ensure first element is a symbol
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression does not start with a symbol!");
  }

  // Call builtin with operator
  lval* result = lval_builtin(v, f->sym);
  lval_del(f);

  return result;
}

lval* lval_builtin(lval* a, char* func) {
  if (strcmp("list", func) == 0) return lval_builtin_list(a);
  if (strcmp("head", func) == 0) return lval_builtin_head(a);
  if (strcmp("tail", func) == 0) return lval_builtin_tail(a);
  if (strcmp("join", func) == 0) return lval_builtin_join(a);
  if (strcmp("eval", func) == 0) return lval_builtin_eval(a);
  if (strcmp("len", func) == 0) return lval_builtin_len(a);
  if (strcmp("+-/*", func)) return lval_builtin_op(a, func);

  lval_del(a);

  return lval_err("Unknown function!");
}

lval* lval_builtin_op(lval* a, char* op) {
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

lval* lval_builtin_head(lval* a) {
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

lval* lval_builtin_tail(lval* a) {
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

lval* lval_builtin_list(lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* lval_builtin_eval(lval* a) {
  // Error conditions
  LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'tail' passed incorrect types!");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;

  return lval_eval(x);
}

lval* lval_builtin_join(lval* a) {
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

lval* lval_builtin_len(lval* a) {
  LASSERT(a, a->count == 1, "Function 'len' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'len' passed incorrect type.");

  lval* x = lval_take(a, 0);
  lval* len = lval_num(x->count);

  lval_del(x);

  return len;
}

lval* lval_eval(lval* v) {
  // Evaluate S-expressions
  if (v->type == LVAL_SEXPR) return lval_eval_sexpr(v);

  return v;
}

lval* lval_join(lval* x, lval* y) {
  // For each cell in 'y' add it to 'x'
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_del(y);
  return x;
}

lval* lval_pop(lval* v, int i) {
  lval* x = v->cell[i];

  // Shift memory after the item at "i" over the top
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval*) * (v->count - i - 1));

  v->count--;

  v->cell = realloc(v->cell, sizeof(lval*) * v->count);

  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);

  return x;
}

void lval_del(lval* v) {
  switch (v->type) {
    case LVAL_NUM:
      break;

    case LVAL_ERR:
      free(v->err);
      break;

    case LVAL_SYM:
      free(v->sym);
      break;

    case LVAL_SEXPR:
    case LVAL_QEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }

      free(v->cell);
      break;
  }

  free(v);
}
