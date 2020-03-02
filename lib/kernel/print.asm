TI_GDT equ 0
RPL0   equ 0
CR_ASCII equ 0x0d
LF_ASCII equ 0x0a
BackSpace_ASCII equ 0x08
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

[bits 32]
section .text
global putChar ; 导出全局符号

putChar:
push ebp
mov ebp, esp 
; 备份所有的32位寄存器
pushad

;安装视频段的段选择子到 gs 寄存器
mov ax, SELECTOR_VIDEO
mov gs, ax 
;  获取光标
mov dx, 0x3d4
mov al, 0x0E
out dx, al ; 设置索引,使其指向光标寄存器, 0x0E 是高8位
mov dx, 0x3d5
in al, dx
mov ah, al

mov dx, 0x3d4
mov al, 0x0F
out dx, al
mov dx, 0x3d5
in al, dx

;把ax 中的光标
mov bx, ax
mov ecx, [ebp + 8] ; 获取 字符参数 也就是 字符的ascii, 1字节 存储在cl

; 当字符是 LF(换行符), CR(回车符), backSpace(退格键)

cmp cl, LF_ASCII
jz DealLF

cmp cl, CR_ASCII
jz DealCR

cmp cl, BackSpace_ASCII
jz DealBackSpace
jmp OtherChar



DealLF:
DealCR:
;这两个在我们的操作系统中是一个效果
xor dx, dx
mov ax, bx
mov si, 80
div si ; 除了之后 dx 中为余数
sub bx, dx

add bx, 80 ; 换行
cmp bx, 2000
jl SetCursor


DealBackSpace:

dec bx ;光标退格
shl bx, 1
mov byte [gs: bx], 0x20 ; 用空格覆盖
inc bx
mov byte [gs: bx], 0x07
shr bx, 1
jmp SetCursor


OtherChar:

shl bx, 1
mov [gs: bx], cl
inc bx
mov byte [gs: bx], 0x07
shr bx, 1
inc bx
jmp SetCursor


;滚屏函数
RollScreen:
;复制数据
cld
xor ecx, ecx
mov ecx, 960
mov edi, 0xc00b8000
mov esi, 0xc00b80a0
rep movsd

;把最后一行全用空格替代
mov ecx, 80
mov ebx, 3840
clear:
mov word [gs : ebx],0x0720
add ebx, 2
loop clear


SetCursor:
mov dx, 0x3d4
mov al, 0x0E
out dx, al 
mov dx, 0x3d5
mov al, bh
out dx, al

mov dx, 0x3d4
mov al, 0x0F
out dx, al
mov dx, 0x3d5
mov al, bl
out dx, al

PutCharDone:
popad
pop ebp
ret

;字符串打印函数
global putStr
;字符串传参是传的地址
putStr:
push ebp
mov ebp, esp
push eax
push esi
mov esi, [ebp + 8] ; 字符串地址

PutStrLoop:
mov al, [esi]
cmp al, 0
jz PutStrDone

push eax
call putChar
add esp,4
inc esi
jmp PutStrLoop

PutStrDone:
pop esi
pop eax
pop ebp
ret

;打印数字函数 putInt(uint32 num)
global putInt
putInt:
push ebp
mov ebp, esp
push eax
push edx
push ecx
push ebx
mov ebx, [ebp + 8] ; 获取数字
; 思路,应该一位一位的打印数字

IntDeal_8:
;四个字节,现在处理最高字节的部分
mov eax, ebx
shr eax, 24
mov cl, 0x10 ; cl现在是 16
div cl ; ax / cl 余数在ah  商在 al
;商 和余数 分别打印
;0x???????? 8位(这里的位不是1比特, 而是十六进制的)

call IntToChar

cmp al, 0
jz IntDeal_7
push ax
call putChar
pop  ax

IntDeal_7:
shr  eax, 8

call IntToChar

cmp al, 0
jz IntDeal_6

push ax
call putChar
pop  ax

IntDeal_6:





pop ebx
pop ecx
pop edx
pop eax

pop ebp

ret

IntToChar:
;这是个辅助例程,就是用来 把al的数字处理一下
isZero:
cmp al, 0
jz  back

lessTen:
cmp al, 0x0a
jge moreTen
add al, 48
jmp back

moreTen:
add al, 87

back:
    ret