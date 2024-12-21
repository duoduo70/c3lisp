gcc -I. -std=gnu99 -O0 -g alloc.c fstr.c parser.c token.c expander.c builtin-func.c main.c -o c3
./c3
nasm -f elf64 test.asm -o testprog.o
ld testprog.o -o testprog
./testprog