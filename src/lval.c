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

  *v = (lval){
      .type = LVAL_NUM,
      .num = x,
  };

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

lval* lval_fun(lbuiltin func) {
  lval* v = malloc(sizeof(lval));

  v->type = LVAL_FUN;
  v->fun = func;

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
    case LVAL_FUN:
      printf("<function>");
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

lval* lval_copy(lval* v) {
  lval* x = malloc(sizeof(lval));
  x->type = v->type;

  switch (v->type) {
    case LVAL_FUN:
      x->fun = v->fun;
      break;

    case LVAL_NUM:
      x->num = v->num;
      break;

    case LVAL_ERR:
      x->err = malloc(strlen(v->err) + 1);
      strcpy(x->err, v->err);
      break;

    case LVAL_SYM:
      x->sym = malloc(strlen(v->sym) + 1);
      strcpy(x->sym, v->sym);
      break;

    case LVAL_SEXPR:
    case LVAL_QEXPR:
      x->count = v->count;
      x->cell = malloc(sizeof(lval*) * x->count);

      for (int i = 0; i < x->count; i++) {
        x->cell[i] = lval_copy(v->cell[i]);
      }

      break;
  }

  return x;
}

lval* lval_eval_sexpr(lenv* e, lval* v) {
  // Eval children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
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
  // Ensure first element is a function
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_FUN) {
    lval_del(f);
    lval_del(v);
    return lval_err("First element is not a function");
  }

  // Call function
  lval* result = f->fun(e, v);
  lval_del(f);

  return result;
}

lval* lval_builtin(lenv* e, lval* a, char* func) {
  if (strcmp("list", func) == 0) return lval_builtin_list(e, a);
  if (strcmp("head", func) == 0) return lval_builtin_head(e, a);
  if (strcmp("tail", func) == 0) return lval_builtin_tail(e, a);
  if (strcmp("join", func) == 0) return lval_builtin_join(e, a);
  if (strcmp("eval", func) == 0) return lval_builtin_eval(e, a);
  if (strcmp("len", func) == 0) return lval_builtin_len(e, a);
  if (strcmp("+-/*", func)) return lval_builtin_op(e, a, func);

  lval_del(a);

  return lval_err("Unknown function!");
}

lval* lval_builtin_op(lenv* e, lval* a, char* op) {
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

lval* lval_builtin_add(lenv* e, lval* a) { return lval_builtin_op(e, a, "+"); }
lval* lval_builtin_sub(lenv* e, lval* a) { return lval_builtin_op(e, a, "-"); }
lval* lval_builtin_mul(lenv* e, lval* a) { return lval_builtin_op(e, a, "*"); }
lval* lval_builtin_div(lenv* e, lval* a) { return lval_builtin_op(e, a, "/"); }

lval* lval_builtin_head(lenv* e, lval* a) {
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

lval* lval_builtin_tail(lenv* e, lval* a) {
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

lval* lval_builtin_list(lenv* e, lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* lval_builtin_eval(lenv* e, lval* a) {
  // Error conditions
  LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'tail' passed incorrect types!");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;

  return lval_eval(e, x);
}

lval* lval_builtin_join(lenv* e, lval* a) {
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

lval* lval_builtin_len(lenv* e, lval* a) {
  LASSERT(a, a->count == 1, "Function 'len' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'len' passed incorrect type.");

  lval* x = lval_take(a, 0);
  lval* len = lval_num(x->count);

  lval_del(x);

  return len;
}

lval* lval_eval(lenv* e, lval* v) {
  if (v->type == LVAL_SYM) {
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }

  if (v->type == LVAL_SEXPR) return lval_eval_sexpr(e, v);

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

    case LVAL_FUN:
      break;
  }

  free(v);
}

lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));

  *e = (lenv){
      .count = 0,
      .syms = NULL,
      .vals = NULL,
  };

  return e;
}

lval* lenv_get(lenv* e, lval* k) {
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }

  return lval_err("unbound symbol!");
}

void lenv_put(lenv* e, lval* k, lval* v) {
  // Check if variable already exists
  for (int i = 0; i < e->count; i++) {
    // and replace if found
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }

  // Otherwise, allocate space for new entry
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);

  // Copy contents
  e->vals[e->count - 1] = lval_copy(v);
  e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
  strcpy(e->syms[e->count - 1], k->sym);
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(func);

  lenv_put(e, k, v);

  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  // List functions
  lenv_add_builtin(e, "list", lval_builtin_list);
  lenv_add_builtin(e, "head", lval_builtin_head);
  lenv_add_builtin(e, "tail", lval_builtin_tail);
  lenv_add_builtin(e, "eval", lval_builtin_eval);
  lenv_add_builtin(e, "join", lval_builtin_join);

  // Mathematical functions
  lenv_add_builtin(e, "+", lval_builtin_add);
  lenv_add_builtin(e, "-", lval_builtin_sub);
  lenv_add_builtin(e, "*", lval_builtin_mul);
  lenv_add_builtin(e, "/", lval_builtin_div);
}

void lenv_del(lenv* e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }

  free(e->syms);
  free(e->vals);
  free(e);
}
