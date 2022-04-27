global __ass_print
global __ass_scan
;global __ass_pow

extern printf
extern scanf

section .text
__ass_print:
        lea rdi, print_fmt
        mov rsi, [rsp + 0x8]
        xor rax, rax
        call printf
        ret
        
section .rodata
print_fmt db `%ld\n\0`

section .text
__ass_scan:
        lea rdi, scan_fmt
        lea rsi, scan
        xor rax, rax
        call scanf
        mov rax, qword [scan]
        ret

section .rodata
scan_fmt db `%ld\0`

section .data
scan dq 0

