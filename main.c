#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>

#include "lval.h"
#include "mpc.h"

int main(int argc, char* argv[]) {
  // Define Polish notation grammar
  // Example: (+ 2 2)

  // Parsers
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Qexpr = mpc_new("qexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  // Grammar
  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                     \
            number : /-?[0-9]+/ ;                                 \
            symbol : \"list\" | \"head\" | \"tail\" | \"join\"    \
                   | \"eval\" | \"len\" | '+' | '-' | '*' | '/' ; \
            sexpr  : '(' <expr>* ')' ;                            \
            qexpr  : '{' <expr>* '}' ;                            \
            expr   : <number> | <symbol> | <sexpr> | <qexpr> ;    \
            lispy  : /^/ <expr>* /$/ ;                            \
            ",
            Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

  puts("Lispy Version 0.1");
  puts("Press ctrl+c to exit\n");

  while (1) {
    // Output prompt and get input
    char* input = readline("lispy> ");

    // Add input to history
    add_history(input);

    // Parse
    mpc_result_t ast;
    if (mpc_parse("<stdin>", input, Lispy, &ast)) {
      lval* x = lval_eval(lval_read(ast.output));
      lval_println(x);
      mpc_ast_delete(ast.output);
    } else {
      mpc_err_print(ast.error);
      mpc_err_delete(ast.error);
    }

    free(input);
  }

  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

  return 0;
}
