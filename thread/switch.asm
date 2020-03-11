[bits 32]

section .text
global switch_to;导出外部符号
;switch_to(struct pcb_struct *curr, struct pcb_struct *next);
switch_to:
;遵循ABI, 保存环境
push esi
push edi
push ebx
push ebp

mov eax, [esp + 20] ; curr指针
mov [eax], esp ; 保存下栈的指针

mov eax, [esp + 24] ; next指针
mov esp, [eax] ;新线程的栈指针

pop ebp
pop ebx
pop edi
pop esi
ret
