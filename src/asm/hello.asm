global _start

section .data:
    msg: db "Hello ASM", 0xA
    msg_len equ $-msg

section .text:

func:
    push ebp
    mov ebp, esp
    mov ebx, DWORD [ebp+8]
    pop ebp
    ret

_start:
    mov eax, 0x4            ; SYSCALL write: /usr/include/asm/unistd_32.h
    mov ebx, 1              ; stdout
    mov ecx, msg            ; buf
    mov edx, msg_len        ; count
    int 0x80                ; syscall interrupt

    push 15
    call func

    mov eax, 0x1            ; SYSCALL exit
    int 0x80
