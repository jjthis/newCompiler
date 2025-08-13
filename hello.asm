; hello.asm
; 빌드: nasm -f win64 hello.asm -o hello64.obj
; 링크: gcc hello64.obj -o hello64.exe -lkernel32 -Wl,-subsystem,console
; hello64.exe

extern GetStdHandle
extern WriteFile
extern ExitProcess

global main

section .data
    msg     db 'Hello, 64-bit World!', 0Dh,0Ah
    msgLen  equ $ - msg

section .bss
    bytesWritten resq 1          ; 8바이트 예약 (64비트 환경)

section .text
main:
    ; 스택 정렬: main 진입 시 rsp % 16 == 8 → 8 더 빼야 정렬
    sub     rsp, 40              ; 32 shadow space + 8 align

    ; HANDLE hStdOut = GetStdHandle(-11)
    mov     ecx, -11
    call    GetStdHandle

    ; WriteFile(hStdOut, msg, msgLen, &bytesWritten, NULL)
    mov     rcx, rax                  ; hStdOut
    lea     rdx, [rel msg]            ; msg 주소
    mov     r8d, msgLen               ; 길이
    lea     r9, [rel bytesWritten]    ; &bytesWritten
    mov     qword [rsp+32], 0         ; lpOverlapped(NULL)
    call    WriteFile

    ; ExitProcess(0)
    xor     ecx, ecx
    call    ExitProcess
