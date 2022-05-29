	;; Simple multiplication tablej
	;;
	;; Tested in:
	;; 	CPU => Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz
	;; 	OS => lubuntu 20.04
	;; 	NASM => 2.14.02
	;; 	Linker => GNU ld (GNU Binutils for Ubuntu) 2.34
	;;
	;; this program structure is similar with following c code:
	;;
	;; #include <stdio.h>
	;; #include <unistd.h>
	;;
	;; #definne STDOUT 0x01
	;; #define EXIT_SUCCESS 0x00
	;;
	;; #define OP_DOMAIN 0x0a
	;; #define LEN 0x02
	;;
	;; int main()
	;; {
	;; 	int op1, op2, res;
	;;
	;;	for (op1 = 1; op1 != OP_DOMAIN; op1++)
	;;		for (op2 = 0; op2 != OP_DOMAIN; ) {
	;;			++op2;
	;;			res = op1 * op2;
	;;			write(1, &res, LEN);
	;;		}
	;;	return 0;
	;; }

	;; data declarations
	section .data

	;; constant definitions
	STDOUT		equ	0x01 ; standard output
	SYS_write	equ	0x01 ; write syscall

	EXIT_SUCCESS	equ	0x00 ; exit flag
	SYS_exit	equ	0x3c ; exit syscall

	;; operation(multiplication) domain
	OP_DOMAIN	equ	0x0a

	;; Byte length definition
	LEN		equ	0x02 ; for readability at hexdump

	;; index definitions
	op1		dw	0x01 ; i = 0x01
	op2		dw	0x00 ; j = 0x00
	res		dw	0x00

	;; code section
	section .text

	global _start
_start:
	jmp _compare_i
_op1:
	;; op2 = 0
	xor rcx, rcx
	mov [op2], cx

	jmp _compare_j
_op2:
	;; op2++
	add cx, 0x01
	mov [op2], cx

	;; res = op1 * op2
	xor rax, rax
	xor rcx, rcx

	mov ax, [op1]
	mov cx, [op2]
	mul cx
	mov [res], ax

_sys_write:
	;; write(STDOUT, &res, LEN) (STDOUT=0x01, LEN=0x02)
	xor rdi, rdi
	xor rsi, rsi
	xor rdx, rdx
	xor rax, rax

	mov rdi, STDOUT
	mov rsi, res
	mov rdx, LEN
	mov al, SYS_write

	syscall

_compare_j:
	;; op2 != 0x0a
	xor rcx, rcx

	mov cx, [op2]
	cmp cx, OP_DOMAIN
	jne _op2

	;; op1++
	xor rcx, rcx

	mov cx, [op1]
	add cx, 0x01
	mov [op1], cx
_compare_i:
	;; op1 != 0x0a
	xor rcx, rcx

	mov cx, [op1]
	cmp cx, OP_DOMAIN
	jne _op1

_sys_exit:
	;; Exit program
	xor rdi, rdi

	mov dl, EXIT_SUCCESS
	mov rax, SYS_exit

	syscall	
