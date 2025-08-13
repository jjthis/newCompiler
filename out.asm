extern printf
extern ExitProcess

global main

section .data
    a dq 0
    b dq 0
fmt_print db "%lld", 0Dh,0Ah, 0

section .text
main:
    sub rsp, 40
    mov rax, 2
    push rax
    mov rax, 3
    push rax
    mov rax, 4
    push rax
    pop rbx
    pop rax
    imul rax, rbx
    push rax
    pop rbx
    pop rax
    add rax, rbx
    push rax
    pop rax
    mov [rel a], rax
    lea rax, [rel a]
    mov rax, [rax]
    push rax
    mov rax, 5
    push rax
    pop rbx
    pop rax
    sub rax, rbx
    push rax
    pop rax
    mov [rel b], rax
    lea rax, [rel a]
    mov rax, [rax]
    push rax
    pop rax
    lea rcx, [rel fmt_print]
    mov rdx, rax
    sub rsp, 40
    call printf
    add rsp, 40
    lea rax, [rel b]
    mov rax, [rax]
    push rax
    mov rax, 2
    push rax
    pop rbx
    pop rax
    imul rax, rbx
    push rax
    mov rax, 3
    push rax
    mov rax, 1
    push rax
    pop rbx
    pop rax
    sub rax, rbx
    push rax
    pop rbx
    pop rax
    add rax, rbx
    push rax
    pop rax
    lea rcx, [rel fmt_print]
    mov rdx, rax
    sub rsp, 40
    call printf
    add rsp, 40
    xor ecx, ecx
    call ExitProcess
