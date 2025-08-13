; hello.asm
; 빌드: nasm -f win64 hello.asm -o hello64.obj
; 링크: gcc hello64.obj -o hello64.exe -lkernel32 -Wl,-subsystem,console
; hello64.exe

extern GetStdHandle
extern WriteFile
extern ExitProcess

; kernel32.dll에 있는 함수들:
; 외부에서 가져올 Windows API 함수 선언.

global main

section .data
    msg     db 'Hello, 64-bit World! dsnjfsd asdasdasd', 0Dh,0Ah
    msgLen  equ $ - msg

section .bss
; .bss는 Block Started by Symbol의 약자.
; 특징:
; 초기화되지 않은 전역 변수를 저장하는 섹션.
; 실제 바이너리 파일에는 데이터가 들어가지 않음 → 대신 길이 정보만 기록.
; 프로그램 실행 시 OS가 이 공간을 0으로 초기화.
; 즉, .data 섹션은 값이 있는 변수 (예: msg db 'Hello')를 넣고,
; .bss는 값이 아직 없는 변수 (예: int a;)를 넣음.
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
    ; msg 라벨(문자열의 시작 위치)의 주소값을 RIP-상대 계산으로 구해 rdx에 담아라.
    ; 즉, rdx ← &msg (포인터).
    ; 이 값이 곧 WriteFile의 2번째 인자 lpBuffer로 전달된다.
    mov     r8d, msgLen               ; 길이
    lea     r9, [rel bytesWritten]    ; &bytesWritten
    mov     qword [rsp+32], 0         ; lpOverlapped(NULL)
    call    WriteFile

    ; ExitProcess(0)
    xor     ecx, ecx
    call    ExitProcess
