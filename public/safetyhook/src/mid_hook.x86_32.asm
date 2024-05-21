bits 32

; save context
push dword [trampoline]
push esp ; push trampoline esp
push esp ; push original esp (this gets fixed later)
push ebp
push eax
push ebx
push ecx
push edx
push esi
push edi
pushfd
sub esp, 128
movdqu [esp+112], xmm7
movdqu [esp+96], xmm6
movdqu [esp+80], xmm5
movdqu [esp+64], xmm4
movdqu [esp+48], xmm3
movdqu [esp+32], xmm2
movdqu [esp+16], xmm1
movdqu [esp], xmm0

; fix stored esp.
mov ecx, [esp+160]
add ecx, 8
mov [esp+160], ecx

; call destination
push esp
call [destination]
add esp, 4

; restore context
movdqu xmm0, [esp]
movdqu xmm1, [esp+16]
movdqu xmm2, [esp+32]
movdqu xmm3, [esp+48]
movdqu xmm4, [esp+64]
movdqu xmm5, [esp+80]
movdqu xmm6, [esp+96]
movdqu xmm7, [esp+112]
add esp, 128
popfd
pop edi
pop esi
pop edx
pop ecx
pop ebx
pop eax
pop ebp
lea esp, [esp+4] ; skip original esp
pop esp
ret

destination:
dd 0
trampoline:
dd 0