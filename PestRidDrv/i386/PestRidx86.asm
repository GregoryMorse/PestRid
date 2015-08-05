.model small, C
.686p

.code
	public  _getidt@4
_getidt@4 PROC\
	idtstruct:ptr fword
sidt idtstruct
ret
_getidt@4 ENDP

	public  _getgdt@4
_getgdt@4 PROC\
	gdtstruct:ptr fword
sgdt gdtstruct
ret
_getgdt@4 ENDP

	public  _getldt@4
_getldt@4 PROC\
	ldtstruct:ptr word
sldt ldtstruct
ret
_getldt@4 ENDP

	public  _gettr@4
_gettr@4 PROC\
	trstruct:ptr word
str trstruct
ret
_gettr@4 ENDP

	public  _getmsw@4
_getmsw@4 PROC\
	mswstruct:ptr word
smsw mswstruct
ret
_getmsw@4 ENDP

	public  _getcr0@4
_getcr0@4 PROC\
	uses eax\
	cr0struct:ptr dword
mov eax, cr0
mov dword ptr [cr0struct], eax
ret
_getcr0@4 ENDP

	public  _getcr2@4
_getcr2@4 PROC\
	uses eax\
	cr2struct:ptr dword
mov eax, cr2
mov dword ptr [cr2struct], eax
ret
_getcr2@4 ENDP

	public  _getcr3@4
_getcr3@4 PROC\
	uses eax\
	cr3struct:ptr dword
mov eax, cr3
mov dword ptr [cr3struct], eax
ret
_getcr3@4 ENDP

	public  _getcr4@4
_getcr4@4 PROC\
	uses eax\
	cr4struct:ptr dword
mov eax, cr4
mov dword ptr [cr4struct], eax
ret
_getcr4@4 ENDP

	public  _getdr0@4
_getdr0@4 PROC\
	uses eax\
	dr0struct:ptr dword
mov eax, dr0
mov dword ptr [dr0struct], eax
ret
_getdr0@4 ENDP

	public  _getdr1@4
_getdr1@4 PROC\
	uses eax\
	dr1struct:ptr dword
mov eax, dr1
mov dword ptr [dr1struct], eax
ret
_getdr1@4 ENDP

	public  _getdr2@4
_getdr2@4 PROC\
	uses eax\
	dr2struct:ptr dword
mov eax, dr2
mov dword ptr [dr2struct], eax
ret
_getdr2@4 ENDP

	public  _getdr3@4
_getdr3@4 PROC\
	uses eax\
	dr3struct:ptr dword
mov eax, dr3
mov dword ptr [dr3struct], eax
ret
_getdr3@4 ENDP

	public  _getdr6@4
_getdr6@4 PROC\
	uses eax\
	dr6struct:ptr dword
mov eax, dr6
mov dword ptr [dr6struct], eax
ret
_getdr6@4 ENDP

	public  _getdr7@4
_getdr7@4 PROC\
	uses eax\
	dr7struct:ptr dword
mov eax, dr7
mov dword ptr [dr7struct], eax
ret
_getdr7@4 ENDP

end