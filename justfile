cc_flags := "-std=c17 -Wall -fsanitize=address" # -Wextra -Wpedantic
libs := "-ledit -lm"

default: build run

run:
    ./target/main

build: init
    cc {{cc_flags}} {{libs}} -g src/*.c -o target/main

release: init
    cc {{cc_flags}} {{libs}} -O3 src/*.c -o target/main

init:
    mkdir -p target/

clean:
    rm -rf target/
