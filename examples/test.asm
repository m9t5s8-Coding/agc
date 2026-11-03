section .data
	str.0: db "Enter a number from 0 - 100: ", 0
	str.1: db "Lower!", 10, 0
	str.2: db "Higher!", 10, 0
	str.3: db "Bingo!", 10, 0


section .text
global _start

_start:
	and rsp, -16
	call main$
	mov rdi, rax
	call __ag_exit

__ag_exit:
	mov rax, 60
	syscall
	ret

__ag_write:
	mov rdx, rsi
	mov rsi, rdi
	mov rdi, 1
	mov rax, 1
	syscall
	ret

__ag_read:
	mov rdx, rsi
	mov rsi, rdi
	mov rdi, 0
	mov rax, 0
	syscall
	ret

__ag_alloc:
	push rbx
	mov rbx, rdi
	lea rsi, [rbx + 16]
	xor edi, edi
	mov edx, 3
	mov r10d, 0x22
	mov r8, -1
	xor r9d, r9d
	mov eax, 9
	syscall
	test rax, rax
	js .alloc_failed
	mov [rax], rbx
	add rax, 16
	pop rbx
	ret
.alloc_failed:
	xor eax, eax
	pop rbx
	ret

__ag_free:
	test rdi, rdi
	jz .free_done
	sub rdi, 16
	mov rsi, [rdi]
	add rsi, 16
	mov eax, 11
	syscall
.free_done:
	ret


__agop_add$int$int:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov qword [rbp - 8], rdi
	mov qword [rbp - 16], rsi
	mov rax, qword [rbp - 8]
	sub rax, qword [rbp - 16]
	mov qword [rbp - 24], rax
	mov rax, qword [rbp - 24]
	leave
	ret

main$:
	push rbp
	mov rbp, rsp
	sub rsp, 112
	mov rax, 48
	mov qword [rbp - 8], rax
	mov rdi, 32
	call __ag_alloc
	mov qword [rbp - 32], rax
	mov rax, qword [rbp - 32]
	mov qword [rbp - 24], rax
	mov rax, 0
	mov qword [rbp - 40], rax

b0:
	mov rax, qword [rbp - 40]
	cmp rax, 0
	sete al
	movzx rax, al
	mov qword [rbp - 48], rax
	mov rax, qword [rbp - 48]
	test rax, rax
	jz b2

b1:
	mov rax, str.0
	mov qword [rbp - 16], rax
	mov rdi, qword [rbp - 16]
	call strlen$string
	mov qword [rbp - 56], rax
	mov rdi, qword [rbp - 16]
	mov rsi, qword [rbp - 56]
	call __ag_write
	mov rdi, qword [rbp - 24]
	mov rsi, 32
	call __ag_read
	mov rdi, qword [rbp - 24]
	call stoi$string
	mov qword [rbp - 72], rax
	mov rax, qword [rbp - 72]
	mov qword [rbp - 64], rax
	mov rax, qword [rbp - 64]
	cmp rax, qword [rbp - 8]
	setg al
	movzx rax, al
	mov qword [rbp - 80], rax
	mov rax, qword [rbp - 80]
	test rax, rax
	jz b4

b3:
	mov rax, str.1
	mov qword [rbp - 16], rax
	mov rdi, qword [rbp - 16]
	call strlen$string
	mov qword [rbp - 88], rax
	mov rdi, qword [rbp - 16]
	mov rsi, qword [rbp - 88]
	call __ag_write
	jmp b5

b4:
	mov rax, qword [rbp - 64]
	cmp rax, qword [rbp - 8]
	setl al
	movzx rax, al
	mov qword [rbp - 96], rax
	mov rax, qword [rbp - 96]
	test rax, rax
	jz b7

b6:
	mov rax, str.2
	mov qword [rbp - 16], rax
	mov rdi, qword [rbp - 16]
	call strlen$string
	mov qword [rbp - 104], rax
	mov rdi, qword [rbp - 16]
	mov rsi, qword [rbp - 104]
	call __ag_write
	jmp b8

b7:
	mov rax, str.3
	mov qword [rbp - 16], rax
	mov rdi, qword [rbp - 16]
	call strlen$string
	mov qword [rbp - 112], rax
	mov rdi, qword [rbp - 16]
	mov rsi, qword [rbp - 112]
	call __ag_write
	mov rax, 1
	mov qword [rbp - 40], rax
	jmp b8

b8:
	jmp b5

b5:
	jmp b0

b2:
	mov rdi, qword [rbp - 16]
	call __ag_free
	mov rdi, qword [rbp - 24]
	call __ag_free
	mov rax, 0
	leave
	ret

strlen$string:
	push rbp
	mov rbp, rsp
	sub rsp, 48
	mov qword [rbp - 8], rdi
	mov rax, 0
	mov qword [rbp - 16], rax

b9:
	mov rax, qword [rbp - 8]
	mov rcx, qword [rbp - 16]
	movzx rax, byte [rax + rcx]
	mov qword [rbp - 24], rax
	mov rax, qword [rbp - 24]
	cmp rax, 0
	setne al
	movzx rax, al
	mov qword [rbp - 32], rax
	mov rax, qword [rbp - 32]
	test rax, rax
	jz b11

b10:
	mov rdi, qword [rbp - 16]
	mov rsi, 1
	call __agop_add$int$int
	mov qword [rbp - 40], rax
	mov rax, qword [rbp - 40]
	mov qword [rbp - 16], rax
	jmp b9

b11:
	mov rax, qword [rbp - 16]
	leave
	ret

stoi$string:
	push rbp
	mov rbp, rsp
	sub rsp, 144
	mov qword [rbp - 8], rdi
	mov rax, 0
	mov qword [rbp - 16], rax
	mov rax, 0
	mov qword [rbp - 24], rax
	mov rdi, qword [rbp - 8]
	call strlen$string
	mov qword [rbp - 40], rax
	mov rax, qword [rbp - 40]
	mov qword [rbp - 32], rax
	mov rax, 0
	mov qword [rbp - 48], rax

b12:
	mov rax, qword [rbp - 24]
	cmp rax, qword [rbp - 32]
	setl al
	movzx rax, al
	mov qword [rbp - 56], rax
	mov rax, qword [rbp - 56]
	test rax, rax
	jz b14

b13:
	mov rax, qword [rbp - 8]
	mov rcx, qword [rbp - 24]
	movzx rax, byte [rax + rcx]
	mov qword [rbp - 64], rax
	mov rax, qword [rbp - 64]
	cmp rax, 57
	setle al
	movzx rax, al
	mov qword [rbp - 72], rax
	mov rax, qword [rbp - 8]
	mov rcx, qword [rbp - 24]
	movzx rax, byte [rax + rcx]
	mov qword [rbp - 80], rax
	mov rax, qword [rbp - 80]
	cmp rax, 48
	setge al
	movzx rax, al
	mov qword [rbp - 88], rax
	mov rax, qword [rbp - 88]
	and rax, qword [rbp - 72]
	mov qword [rbp - 96], rax
	mov rax, qword [rbp - 96]
	test rax, rax
	jz b16

b15:
	mov rax, qword [rbp - 16]
	imul rax, 10
	mov qword [rbp - 104], rax
	mov rax, qword [rbp - 8]
	mov rcx, qword [rbp - 24]
	movzx rax, byte [rax + rcx]
	mov qword [rbp - 112], rax
	mov rax, qword [rbp - 112]
	sub rax, 48
	mov qword [rbp - 120], rax
	mov rdi, qword [rbp - 104]
	mov rsi, qword [rbp - 120]
	call __agop_add$int$int
	mov qword [rbp - 128], rax
	mov rax, qword [rbp - 128]
	mov qword [rbp - 16], rax
	jmp b16

b16:
	mov rax, qword [rbp - 24]
	mov qword [rbp - 136], rax
	mov rax, qword [rbp - 24]
	add rax, 1
	mov qword [rbp - 144], rax
	mov rax, qword [rbp - 144]
	mov qword [rbp - 24], rax
	jmp b12

b14:
	mov rax, qword [rbp - 16]
	leave
	ret

to_string$int:
	push rbp
	mov rbp, rsp
	sub rsp, 224
	mov qword [rbp - 8], rdi
	mov rdi, 32
	call __ag_alloc
	mov qword [rbp - 24], rax
	mov rax, qword [rbp - 24]
	mov qword [rbp - 16], rax
	mov rax, qword [rbp - 8]
	mov qword [rbp - 32], rax
	mov rax, 0
	mov qword [rbp - 40], rax
	mov rax, 0
	mov qword [rbp - 48], rax
	mov rax, 0
	mov qword [rbp - 56], rax
	mov rax, 0
	mov qword [rbp - 64], rax
	mov rax, 0
	mov qword [rbp - 72], rax
	mov rax, 0
	mov qword [rbp - 80], rax
	mov rax, qword [rbp - 32]
	cmp rax, 0
	sete al
	movzx rax, al
	mov qword [rbp - 88], rax
	mov rax, qword [rbp - 88]
	test rax, rax
	jz b18

b17:
	mov rax, qword [rbp - 16]
	mov rcx, 0
	mov rdx, 48
	mov byte [rax + rcx], dl
	mov rax, qword [rbp - 16]
	mov rcx, 1
	mov rdx, 0
	mov byte [rax + rcx], dl
	mov rax, qword [rbp - 16]
	leave
	ret
	jmp b18

b18:
	mov rax, qword [rbp - 32]
	cmp rax, 0
	setl al
	movzx rax, al
	mov qword [rbp - 96], rax
	mov rax, qword [rbp - 96]
	test rax, rax
	jz b20

b19:
	mov rax, 1
	mov qword [rbp - 48], rax
	jmp b20

b20:

b21:
	mov rax, qword [rbp - 32]
	cmp rax, 0
	setne al
	movzx rax, al
	mov qword [rbp - 104], rax
	mov rax, qword [rbp - 104]
	test rax, rax
	jz b23

b22:
	mov rax, qword [rbp - 32]
	mov rcx, 10
	cqo
	idiv rcx
	mov rax, rdx
	mov qword [rbp - 112], rax
	mov rax, qword [rbp - 112]
	mov qword [rbp - 56], rax
	mov rax, qword [rbp - 56]
	cmp rax, 0
	setl al
	movzx rax, al
	mov qword [rbp - 120], rax
	mov rax, qword [rbp - 120]
	test rax, rax
	jz b25

b24:
	mov rax, 0
	sub rax, qword [rbp - 56]
	mov qword [rbp - 128], rax
	mov rax, qword [rbp - 128]
	mov qword [rbp - 56], rax
	jmp b25

b25:
	mov rdi, 48
	mov rsi, qword [rbp - 56]
	call __agop_add$int$int
	mov qword [rbp - 136], rax
	mov rax, qword [rbp - 16]
	mov rcx, qword [rbp - 40]
	mov rdx, qword [rbp - 136]
	mov byte [rax + rcx], dl
	mov rdi, qword [rbp - 40]
	mov rsi, 1
	call __agop_add$int$int
	mov qword [rbp - 144], rax
	mov rax, qword [rbp - 144]
	mov qword [rbp - 40], rax
	mov rax, qword [rbp - 32]
	mov rcx, 10
	cqo
	idiv rcx
	mov qword [rbp - 152], rax
	mov rax, qword [rbp - 152]
	mov qword [rbp - 32], rax
	jmp b21

b23:
	mov rax, qword [rbp - 48]
	cmp rax, 1
	sete al
	movzx rax, al
	mov qword [rbp - 160], rax
	mov rax, qword [rbp - 160]
	test rax, rax
	jz b27

b26:
	mov rax, qword [rbp - 16]
	mov rcx, qword [rbp - 40]
	mov rdx, 45
	mov byte [rax + rcx], dl
	mov rdi, qword [rbp - 40]
	mov rsi, 1
	call __agop_add$int$int
	mov qword [rbp - 168], rax
	mov rax, qword [rbp - 168]
	mov qword [rbp - 40], rax
	jmp b27

b27:
	mov rax, qword [rbp - 16]
	mov rcx, qword [rbp - 40]
	mov rdx, 0
	mov byte [rax + rcx], dl
	mov rax, 0
	mov qword [rbp - 64], rax
	mov rax, qword [rbp - 40]
	sub rax, 1
	mov qword [rbp - 176], rax
	mov rax, qword [rbp - 176]
	mov qword [rbp - 72], rax

b28:
	mov rax, qword [rbp - 64]
	cmp rax, qword [rbp - 72]
	setl al
	movzx rax, al
	mov qword [rbp - 184], rax
	mov rax, qword [rbp - 184]
	test rax, rax
	jz b30

b29:
	mov rax, qword [rbp - 16]
	mov rcx, qword [rbp - 64]
	movzx rax, byte [rax + rcx]
	mov qword [rbp - 192], rax
	mov rax, qword [rbp - 192]
	mov qword [rbp - 80], rax
	mov rax, qword [rbp - 16]
	mov rcx, qword [rbp - 72]
	movzx rax, byte [rax + rcx]
	mov qword [rbp - 200], rax
	mov rax, qword [rbp - 16]
	mov rcx, qword [rbp - 64]
	mov rdx, qword [rbp - 200]
	mov byte [rax + rcx], dl
	mov rax, qword [rbp - 16]
	mov rcx, qword [rbp - 72]
	mov rdx, qword [rbp - 80]
	mov byte [rax + rcx], dl
	mov rdi, qword [rbp - 64]
	mov rsi, 1
	call __agop_add$int$int
	mov qword [rbp - 208], rax
	mov rax, qword [rbp - 208]
	mov qword [rbp - 64], rax
	mov rax, qword [rbp - 72]
	sub rax, 1
	mov qword [rbp - 216], rax
	mov rax, qword [rbp - 216]
	mov qword [rbp - 72], rax
	jmp b28

b30:
	mov rax, qword [rbp - 16]
	leave
	ret
