;
; Tomato OS
; [Internal] 设置 RSP 并跳转
; (c) 2015 SunnyCase
; 创建日期：2015-4-22

.CODE

setrspjmp PROC
	mov rsp, rcx
	jmp rdx
setrspjmp ENDP

END