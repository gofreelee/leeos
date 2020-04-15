TI_GDT equ 0
RPLO equ 0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPLO

section .data
put_int_buffer dq 0		; 定义8字节缓冲区用于数字到字符的转换

[bits 32]
section .text

global cls_screen
cls_screen:
	pushad
	;;;;;;;;;;;;;;;
	; 由于用户程序的cpl为3,显存段的dpl为0,故用于显存段的选择子gs在低于自己特权的环境中为0,
	; 导致用户程序再次进入中断后,gs为0,故直接在put_str中每次都为gs赋值. 
	mov ax, SELECTOR_VIDEO	; 不能直接把立即数送入gs,须由ax中转
	mov gs, ax

	mov ebx, 0
	mov ecx, 80*25
.cls:
	mov word [gs:ebx], 0x0720	;0x0720是黑底白字的空格键
	add ebx, 2
	loop .cls 
	mov ebx, 0

.set_cursor:				;直接把set_cursor搬过来用,省事
;;;;;;; 1 先设置高8位 ;;;;;;;;
	mov dx, 0x03d4			;索引寄存器
	mov al, 0x0e			;用于提供光标位置的高8位
	out dx, al
	mov dx, 0x03d5			;通过读写数据端口0x3d5来获得或设置光标位置 
	mov al, bh
	out dx, al

;;;;;;; 2 再设置低8位 ;;;;;;;;;
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5 
	mov al, bl
	out dx, al
	popad
	ret

;---------------------put_int----------------------
; 将小端数字打印到屏幕上, 打印16进制的(不带0x)
;---------------------------------------------------
global putInt
putInt:
	pushad			; 备份所有寄存器
	mov ebp, esp
	mov eax, [ebp + 4 * 9]		; call的返回地址占4字节, pushad指令的8 * 4字节
	mov edx, eax		
	mov edi, 7		; 指定put_int_buffer中的偏移量
	mov ecx, 8		; 32位数字中, 十六进制数字的位数是8个
	mov ebx, put_int_buffer

; 每4位是1位十六进制数字
.16based_4bits:
	and edx, 0x0000000f			; 保留最后4位数, and是与操作

	cmp edx, 9					; 判断是0~9还是a~f
	jg .is_A2F
	add edx, '0'				; ASCII码是8位大小, add求和后, 只有低8位有效
	jmp .store

.is_A2F:
	sub edx, 10					; 减去10再加上'A'的值, 就是对应的ASCII值
	add edx, 'A'				; 例如: 0xf - 10 = 5 + 'A' == 'F'

; 反序, 变成大端存储, 符合人的认知
.store:
	mov [ebx + edi], dl
	dec edi
	shr eax, 4					; 当最后4位处理完了, 右移4位
	mov edx, eax
	loop .16based_4bits

; 此时put_int_buffer中全是字符, 把高位连续的字符去掉
.ready_to_print:
	inc edi						; 此时edi已经为-1(循环了8次, 7 - 8 == 01), 使其++
.skip_prefix_0:
	cmp edi, 8					; 若已经比较到第9个字符了, 打印字符全部为0

	je .full0					; 有道理

.go_on_skip:
	mov cl, [put_int_buffer + edi]
	inc edi
	cmp cl, '0'
	je .skip_prefix_0		; 判断下一位字符是否为字符0
	dec edi
	jmp .put_each_num

.full0:
	mov cl, '0'				; 输入的数字全为0时, 打印0

.put_each_num:
	push ecx				; cl为可打印的字符
	call putChar
	add esp, 4
	inc edi
	mov cl, [put_int_buffer + edi]
	cmp edi, 8
	jl .put_each_num
	popad
	ret

;---------------------put_str----------------------
; 通过调用putChar来打印以0结尾的字符串
;---------------------------------------------------
global putStr
putStr:
	push ebx		; 只用到ebx, ecx两个寄存器, 就只备份这两个
	push ecx

	xor ecx, ecx
	mov ebx, [esp + 12]	; 从栈中拿到字符串

.goon:
	mov cl, [ebx]
	cmp cl, 0			; 判断是否结束
	jz .str_over
	push ecx			; 为putChar传递参数
	call putChar
	add esp, 4			; 回收参数所占的栈空间
	inc ebx				; 指向下一个字符
	jmp .goon

.str_over:
	pop ecx
	pop ebx
	ret

;---------------------putChar----------------------
; 把栈中的一个字符写入光标所在处
;---------------------------------------------------
global putChar
putChar:
	pushad			; 备份32位寄存器环境
	mov ax, SELECTOR_VIDEO
	mov gs, ax


	; 获取当前光标位置
	mov dx, 0x03d4		; 索引寄存器
	mov al, 0x0e		; 提供光标的高8位
	out dx, al
	mov dx, 0x03d5		; 读写该端口获取或设置光标位置
	in al, dx			; 得到光标高8位
	mov ah, al


	; 再得到低8位
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	in al, dx

	; 将光标存入bx
	mov bx, ax

	; 拿到待打印的字符
	mov ecx, [esp + 36]		; pushad压入4 * 8 == 32字节
							; 加上主调函数的返回地址, 故esp + 36
	cmp cl, 0xd			; CR是0x0d, LF是0x0a
	jz .is_carriage_return
	cmp cl, 0xa
	jz .is_line_feed

	cmp cl, 0x8			; BS(backspace)的ascii码是8
	jz .is_backspace
	jmp .put_other

.is_backspace:
	; 看书上P280
	dec bx
	shl bx, 1

	mov byte [gs:bx], 0x20
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1
	jmp .set_cursor

.put_other:
	shl bx, 1

	mov [gs: bx], cl
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1
	inc bx
	cmp bx, 2000			; 超出2000个字符了
	jl .set_cursor


.is_line_feed:			; \n
.is_carriage_return:	; \r	将光标移到行首即可

	xor dx, dx
	mov ax, bx
	mov ax, bx
	mov si, 80

	div si
	sub bx, dx

.is_carriage_return_end:		; 回车符CR处理结束
	add bx, 80
	cmp bx, 2000
.is_line_feed_end:				; 若是LF, 将光标 + 80即可
	jl .set_cursor



; 屏幕范围是0~24, 滚屏原理就是将1~24行搬运到0~23, 覆盖第0行, 就空了第24行
.roll_screen:
	cld				; 清除方向位
	mov ecx, 960	; 一次搬运4字节, 共960次

	mov esi, 0xc00b80a0		; 第一行行首
	mov edi, 0xc00b8000		; 第零行行首
	rep movsd

	; 将最后一行填充空白
	mov ebx, 3840
	mov ecx, 80

.cls:
	mov word [gs:ebx], 0x0720	; 黑底白字的空格键
	add ebx, 2
	loop .cls
	mov bx, 1920				; 将光标重置为1920

.set_cursor:
; 设置光标位置
	mov dx, 0x03d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x03d5
	mov al, bh
	out dx, al

	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	mov al, bl
	out dx, al

.putChar_done:
	popad
	ret


global set_cursor
set_cursor:
   pushad
   mov bx, [esp+36]
;;;;;;; 1 先设置高8位 ;;;;;;;;
   mov dx, 0x03d4			  ;索引寄存器
   mov al, 0x0e				  ;用于提供光标位置的高8位
   out dx, al
   mov dx, 0x03d5			  ;通过读写数据端口0x3d5来获得或设置光标位置 
   mov al, bh
   out dx, al

;;;;;;; 2 再设置低8位 ;;;;;;;;;
   mov dx, 0x03d4
   mov al, 0x0f
   out dx, al
   mov dx, 0x03d5 
   mov al, bl
   out dx, al
   popad
   ret