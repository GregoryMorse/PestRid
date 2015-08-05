.code
;first argument passed in ecx
	public  _getidt
_getidt PROC
sidt fword ptr [rcx]
ret
_getidt ENDP

	public  _getgdt
_getgdt PROC
sgdt fword ptr [rcx]
ret
_getgdt ENDP

	public  _getldt
_getldt PROC
sldt word ptr [rcx]
ret
_getldt ENDP

	public  _gettr
_gettr PROC
str word ptr [rcx]
ret
_gettr ENDP

	public  _getmsw
_getmsw PROC
smsw word ptr [rcx]
ret
_getmsw ENDP

	public _getcr0
_getcr0 PROC uses rax
mov rax, cr0
mov qword ptr [rcx], rax
ret
_getcr0 ENDP

	public _getcr2
_getcr2 PROC uses rax
mov rax, cr2
mov qword ptr [rcx], rax
ret
_getcr2 ENDP

	public _getcr3
_getcr3 PROC uses rax
mov rax, cr3
mov qword ptr [rcx], rax
ret
_getcr3 ENDP

	public _getcr4
_getcr4 PROC uses rax
mov rax, cr4
mov qword ptr [rcx], rax
ret
_getcr4 ENDP

	public _getcr8
_getcr8 PROC uses rax
mov rax, cr8
mov qword ptr [rcx], rax
ret
_getcr8 ENDP

	public _getdr0
_getdr0 PROC uses rax
mov rax, dr0
mov qword ptr [rcx], rax
ret
_getdr0 ENDP

	public _getdr1
_getdr1 PROC uses rax
mov rax, dr1
mov qword ptr [rcx], rax
ret
_getdr1 ENDP

	public _getdr2
_getdr2 PROC uses rax
mov rax, dr2
mov qword ptr [rcx], rax
ret
_getdr2 ENDP

	public _getdr3
_getdr3 PROC uses rax
mov rax, dr3
mov qword ptr [rcx], rax
ret
_getdr3 ENDP

	public _getdr6
_getdr6 PROC uses rax
mov rax, dr6
mov qword ptr [rcx], rax
ret
_getdr6 ENDP

	public _getdr7
_getdr7 PROC uses rax
mov rax, dr7
mov qword ptr [rcx], rax
ret
_getdr7 ENDP

end