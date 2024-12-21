[bits 64]
global _start

section .text
_start:
jmp main
f1:
mov rax,1
mov rdi,1
mov rsi,[rsp+16]
mov rdx,[rsp+8]
syscall
ret
f2:mov rax,60
mov rdi,0
syscall
main:
call f2
section .data
