(set^ print (asm
  [arg #msg
       #len]
  [mov rax 1]
  [mov rdi 1]
  [mov rsi #msg]
  [mov rdx #len]
  [syscall]
  [ret]))

(set^ exit (asm
  [mov rax 60]
  [mov rdi 0]
  [syscall]))

(set! print (# (s) (call^ print s (len s))))

(call^ exit)