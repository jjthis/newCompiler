nasm -f win64 out.asm -o hello64.obj
gcc hello64.obj -o hello64.exe -lkernel32 -Wl,-subsystem,console
hello64.exe
pause