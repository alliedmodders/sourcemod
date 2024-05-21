bits 64

; save context
push qword [rel trampoline]
push rsp ; push trampoline rsp
push rsp ; push original rsp (this gets fixed later)
push rbp
push rax
push rbx
push rcx
push rdx
push rsi
push rdi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
pushfq
sub rsp, 256
movdqu [rsp+240], xmm15
movdqu [rsp+224], xmm14
movdqu [rsp+208], xmm13
movdqu [rsp+192], xmm12
movdqu [rsp+176], xmm11
movdqu [rsp+160], xmm10
movdqu [rsp+144], xmm9
movdqu [rsp+128], xmm8
movdqu [rsp+112], xmm7
movdqu [rsp+96], xmm6
movdqu [rsp+80], xmm5
movdqu [rsp+64], xmm4
movdqu [rsp+48], xmm3
movdqu [rsp+32], xmm2
movdqu [rsp+16], xmm1
movdqu [rsp], xmm0

; fix stored rsp.
mov rcx, [rsp+384]
add rcx, 16
mov [rsp+384], rcx

; set destination parameter
lea rcx, [rsp]

; align stack, save original
mov rbx, rsp
sub rsp, 48
and rsp, -16

; call destination
call [rel destination]

; restore stack
mov rsp, rbx

; restore context
movdqu xmm0, [rsp]
movdqu xmm1, [rsp+16]
movdqu xmm2, [rsp+32]
movdqu xmm3, [rsp+48]
movdqu xmm4, [rsp+64]
movdqu xmm5, [rsp+80]
movdqu xmm6, [rsp+96]
movdqu xmm7, [rsp+112]
movdqu xmm8, [rsp+128]
movdqu xmm9, [rsp+144]
movdqu xmm10, [rsp+160]
movdqu xmm11, [rsp+176]
movdqu xmm12, [rsp+192]
movdqu xmm13, [rsp+208]
movdqu xmm14, [rsp+224]
movdqu xmm15, [rsp+240]
add rsp, 256
popfq
pop r15
pop r14
pop r13
pop r12
pop r11
pop r10
pop r9
pop r8
pop rdi
pop rsi
pop rdx
pop rcx
pop rbx
pop rax
pop rbp
lea rsp, [rsp+8] ; skip original rsp
pop rsp
ret

destination:
dq 0
trampoline:
dq 0