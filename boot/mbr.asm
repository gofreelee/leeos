%include "boot.inc"
section MBR vstart=0x7c00
mov ax, cs
mov ds, ax
mov ss, ax
mov fs, ax
mov es, ax
mov sp, 0x7c00
mov ax, 0xb800
mov gs, ax

;上面完成了初始化
mov ax, 0x0600
mov bx, 0x0700
mov cx, 0x0000
mov dx, 0x184f

int 0x10
;上面是清屏

;从硬盘加载 loader
mov cx ,4
mov eax, loader_start_sector
mov ebx , loader_base_addr
call readDisk   


jmp loader_base_addr ; 移交控制权

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
    mov [bx], ax
    add bx, 2
    loop readData
    ret

times 510 -($-$$) db 0
db 0x55,0xaa