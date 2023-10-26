#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc.h"

int main(int argc, char *argv[]) {
  // Define Polish notation grammar
  // Example: (+ 2 2)

  // Parsers
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispy = mpc_new("lispy");

  // Grammar
  mpca_lang(MPCA_LANG_DEFAULT, "\
            number   : /-?[0-9]+/ ;                            \
            operator : '+' | '-' | '*' | '/' ;                 \
            expr     : <number> | '(' <operator> <expr>+ ')' ; \
            lispy    : /^/ <operator> <expr>+ /$/ ;            \
            ",
            Number, Operator, Expr, Lispy);

  puts("Lispy Version 0.1");
  puts("Press ctrl+c to exit\n");

  while (1) {
    // Output prompt and get input
    char *input = readline("lispy> ");

    // Add input to history
    add_history(input);

    // Parse
    mpc_result_t result;
    if (mpc_parse("<stdin>", input, Lispy, &result)) {
      mpc_ast_print(result.output);
      mpc_ast_delete(result.output);
    } else {
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}
