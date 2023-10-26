cc_flags := "-std=c17 -Wall -Wextra -Wpedantic -fsanitize=address"
libs := "-ledit -lm"

default: run

run: build
    ./main

build:
    cc {{cc_flags}} {{libs}} -g *.c -o main

clean:
    rm -rf main *.dSYM
