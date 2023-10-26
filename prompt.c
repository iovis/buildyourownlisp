#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

int main(int argc, char *argv[]) {
  puts("Lispy Version 0.1");
  puts("Press ctrl+c to exit\n");

  while (1) {
    // Output prompt and get input
    char *input = readline("lispy> ");

    // Add input to history
    add_history(input);

    // Echo
    printf("No, you're a %s\n", input);

    free(input);
  }

  return 0;
}
