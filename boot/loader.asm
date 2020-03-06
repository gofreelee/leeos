%include "boot.inc"
section Loader vstart=loader_base_addr
STACK_TOP equ loader_base_addr;堆栈顶的地址
jmp loader_start

gdt_start:
;gdt开始的占位用的
    dd 0x00000000
    dd 0x00000000
;代码段
code_seg:
    dd 0x0000ffff
    dd DESC_CODE_HIGH4
;数据段
data_seg:
    dd 0x0000ffff
    dd DESC_DATA_HIGH4
;指代显存的区域的段
video_seg:
    dd 0x80000007
    dd DESC_VIDEO_HIGH4
gdt_length equ $-gdt_start
gdt_limit  equ gdt_length - 1
;一些选择子
CODE_SELECT equ (0x0001 << 3) + TI_GDT + RPL0
DATA_SELECT equ (0x0002 << 3) + TI_GDT + RPL0
VIDEO_SELECT equ (0x0003 << 3) + TI_GDT + RPL0
; 预留描述符
times 59 dq 0 ; 到此64 * 8 = 512 字节
dd 0
db 0
memory_size:
 dd 0 ; 用来存放内存的大小

gdt_config:
    dw gdt_limit
    dd gdt_start

ards_bufs times 244 db 0 ; 存放ards的区域
ards_length dw 0 ; ards 的有效长度
; 8 + 240 + 8 = 256,搞个对齐
loader_start:
    mov di, ards_bufs
    mov edx, 0x534d4150
    xor ebx, ebx
E820loop:
    mov eax, 0x0000E820
    mov ecx, 0x00000014 ; 20
    int 0x15
    jc  E820faile_so_E821
    add di, 20
    inc word [ards_length]
    cmp ebx, 0
    jnz E820loop

;至此,读取了信息到内存中,下面进行信息提取,找出最大的内存容量

    mov bx, ards_length
    mov cx, [bx] ; 循环次数
    mov ebx, ards_bufs
    mov eax, 0
find_memory_size:
    mov edx, [ebx]
    add edx, [ebx + 0x08]
    add ebx, 0x00000014
    cmp eax, edx
    jge change_curr_max
    mov eax, edx
change_curr_max:
    loop find_memory_size
    jmp set_memory_size
E820faile_so_E821:





set_memory_size:
    mov [memory_size], eax


    in al, 0x92
    or al, 00000010b
    out 0x92, al
    
    ;cr0的最后一位设置为0,进入保护模式
    lgdt [gdt_config]

    mov eax,cr0
    or  eax,0x00000001
    mov cr0,eax
    jmp dword CODE_SELECT:protect_mode_start ;此时已经在保护模式了
[bits 32]
protect_mode_start:
    mov ax, DATA_SELECT
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, STACK_TOP
    mov ax, VIDEO_SELECT
    mov gs, ax

    mov ebx, KERNEL_BIN_ADD_BASE
    mov eax, KERNEL_START_SECTOR
    mov ecx , 200
    call readDisk


    call set_page
    sgdt [gdt_config]
    mov ebx,[gdt_config + 2]
    or dword [ebx + 0x18 + 4], 0xC0000000
    add dword [gdt_config + 2], 0xC0000000
    add esp, 0xC0000000
    mov eax, PAGE_DIR_TABLE
    mov cr3, eax
    mov eax, cr0
    or  eax, 0x80000000
    mov cr0, eax

    lgdt [gdt_config]
    ;已经开起了分页
    call init_kernel
    mov esp, 0xc009f000
    jmp KERNEL_ENTRY_POINT
set_page:
    mov ecx , 4096
    mov esi , 0
    mov ebx , PAGE_DIR_TABLE
init_dir_table:
    ;这个函数用来把页目录表先清零
    mov byte [ebx + esi], 0
    inc esi
    loop init_dir_table
set_pde:
    ;先把0x00000000 和 0xC0000000 地址对应的设置了
    mov eax, PAGE_DIR_TABLE
    add eax, 0x1000
    or  eax, PAGE_P|PAGE_RW_ONE|PAGE_US_ONE
    mov [PAGE_DIR_TABLE], eax
    mov esi, 768
    mov [ebx + esi * 4], eax
    sub eax, 0x1000
    mov [ebx + 4092], eax ; 让页目录的最后一个指向自己
set_pte:
    ;设置页表
    ;第一个页表的位置 PAGE_DIR_TABLE + 0x1000,因为考虑到内核只有1MB
    ;设置页表也只是需要第一个页表的前256个页表项
    add ebx, 0x1000 ;指令执行之后,ebx中存储的就是第一个页表的位置
    mov esi, 0
    mov ecx, 256
    mov edx, 0x00000000
    or  edx, PAGE_P|PAGE_RW_ONE|PAGE_US_ONE
set_pte_loop:
    mov [ebx + esi * 4], edx
    inc esi
    add edx, 0x00001000
    loop set_pte_loop
    ;下面需要把0xC0000000往上所有的地址在页目录表中设置映射
    ;把页目录表偏移处为(PAGE_DIR_TABLE + 769 * 4) ~ (PAGE_DIR_TABLE + 1022 * 4)
    ;地址空间最后4MB 另有他用
    mov ebx, PAGE_DIR_TABLE
    mov esi, 769
    mov ecx, 254
    mov edx, PAGE_DIR_TABLE
    add edx, 0x00002000
    or  edx, PAGE_P|PAGE_RW_ONE|PAGE_US_ONE
set_other_pde:
    mov [ebx + esi * 4], edx
    inc esi
    add edx, 0x00001000
    loop set_other_pde
    ret


;从硬盘读取数据的例程
readDisk:
;先设置sector count ,ebx: loader 地址 eax: loader的扇区 cx: sector count
mov esi, eax
mov di, cx

mov dx, 0x1F2
mov ax, cx
out dx, al

mov eax, esi
;  LBA low
mov cx, 0x08
mov dx, 0x1F3
out dx, al
; lBA mid
shr eax, cl
mov dx, 0x1F4
out dx, al
; LBA high
shr eax, cl
mov dx, 0x1F5
out dx, al
; device
shr eax, cl
and al, 0x0  
or al, 0xE0
mov dx, 0x1F6
out dx, al
;然后进行写命令
mov dx, 0x1F7
; 0x20是读命令
mov al, 0x20
out dx, al
;下面要检测硬盘状态了

notReady:
    nop
    mov dx, 0x1F7
    in  al, dx
    and al, 0x88
    cmp al, 0x80
    jz notReady
;下面是准备好了,开始读数据
mov ax,di
mov dx, 256
mul dx
mov cx, ax
mov dx, 0x1F0

readData:
    in ax, dx
    mov [ebx], ax
    add ebx, 2
    loop readData
ret

init_kernel:
    ;在执行这个函数之前应该已经把 kernel.bin 加载到 KERNEL_BIN_ADD_BASE 中了
    ;下面要把四个寄存器初始化
    xor eax,eax
    xor ebx,ebx
    xor ecx,ecx
    xor edx,edx
    ;程序头表的入口处
    mov ebx, [KERNEL_BIN_ADD_BASE + 28]
    add ebx, KERNEL_BIN_ADD_BASE ; 此时ebx 指向 第一个程序头表
    mov  cx, [KERNEL_BIN_ADD_BASE + 44] ; ecx 存储 有多少程序头表
    mov  dx, [KERNEL_BIN_ADD_BASE + 42] ; 一个程序头表的长度
find_useful_program:
    cmp byte[ebx], P_NULL
    je  Deal_P_NULL
    push dword [ebx + 16] ; size 参数
    push dword [ebx + 8 ] ; dst ,虚拟地址
    mov  eax,  [ebx + 4 ]
    add  eax,  KERNEL_BIN_ADD_BASE; src 地址
    push eax

    call mem_cpy
    add esp, 12 

Deal_P_NULL:
    add ebx, edx
    loop find_useful_program
    ret


mem_cpy:
    ;从左到右的参数 src,dst,size
    cld 
    push ebp ; 保存 ebp
    mov ebp, esp
    push ecx

    mov edi, [ebp + 12]
    mov esi, [ebp + 8 ]
    mov ecx, [ebp + 16]
    rep movsb
    pop ecx
    pop ebp
    ret    