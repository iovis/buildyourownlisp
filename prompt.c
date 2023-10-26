#include <stdio.h>

// Buffer for user input
#define BUFFER_SIZE 2048
static char input[BUFFER_SIZE];

int main(int argc, char *argv[]) {
  puts("Lispy Version 0.1");
  puts("Press ctrl+c to exit\n");

  while (1) {
    // Prompt
    fputs("lispy> ", stdout);

    // Read line
    fgets(input, BUFFER_SIZE, stdin);

    // Echo
    printf("No, you're a %s", input);
  }

  return 0;
}
