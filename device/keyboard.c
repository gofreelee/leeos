#include "../kernel/global.h"
#include "../kernel/io.h"
#include "../kernel/interrupt.h"
#include "../lib/kernel/print.h"

#define KBD_BUF_PORT 0x60 //键盘buffer寄存器的端口号

/*一些控制字符没有ascii码，并且不能显示，用\xxx表示,或者别的转义 */
#define esc '\033'
#define backspace '\b'
#define tab '\t'
#define enter '\r'
#define delete '\177'

#define char_invisible 0
#define ctrl_l_char char_invisible
#define ctrl_r_char char_invisible
#define shift_l_char char_invisible
#define shift_r_char char_invisible
#define alt_l_char char_invisible
#define alt_r_char char_invisible
#define caps_lock_char char_invisible

#define shift_l_make 0x2a
#define shift_r_make 0x36
#define alt_l_make 0x38
#define alt_r_make 0xe038
#define alt_r_break 0xe0b8
#define ctrl_l_make 0x1d
#define ctrl_r_make 0xe01d
#define ctrl_r_break 0xe09d
#define caps_lock_make 0x3a
#define bool int

static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

/**
 * 以通码make_code为索引的二维数组
 */
static char keymap[][2] = {
    /* 扫描码   未与shift组合  与shift组合*/
    /* ---------------------------------- */
    /* 0x00 */ {0, 0},
    /* 0x01 */ {esc, esc},
    /* 0x02 */ {'1', '!'},
    /* 0x03 */ {'2', '@'},
    /* 0x04 */ {'3', '#'},
    /* 0x05 */ {'4', '$'},
    /* 0x06 */ {'5', '%'},
    /* 0x07 */ {'6', '^'},
    /* 0x08 */ {'7', '&'},
    /* 0x09 */ {'8', '*'},
    /* 0x0A */ {'9', '('},
    /* 0x0B */ {'0', ')'},
    /* 0x0C */ {'-', '_'},
    /* 0x0D */ {'=', '+'},
    /* 0x0E */ {backspace, backspace},
    /* 0x0F */ {tab, tab},
    /* 0x10 */ {'q', 'Q'},
    /* 0x11 */ {'w', 'W'},
    /* 0x12 */ {'e', 'E'},
    /* 0x13 */ {'r', 'R'},
    /* 0x14 */ {'t', 'T'},
    /* 0x15 */ {'y', 'Y'},
    /* 0x16 */ {'u', 'U'},
    /* 0x17 */ {'i', 'I'},
    /* 0x18 */ {'o', 'O'},
    /* 0x19 */ {'p', 'P'},
    /* 0x1A */ {'[', '{'},
    /* 0x1B */ {']', '}'},
    /* 0x1C */ {enter, enter},
    /* 0x1D */ {ctrl_l_char, ctrl_l_char},
    /* 0x1E */ {'a', 'A'},
    /* 0x1F */ {'s', 'S'},
    /* 0x20 */ {'d', 'D'},
    /* 0x21 */ {'f', 'F'},
    /* 0x22 */ {'g', 'G'},
    /* 0x23 */ {'h', 'H'},
    /* 0x24 */ {'j', 'J'},
    /* 0x25 */ {'k', 'K'},
    /* 0x26 */ {'l', 'L'},
    /* 0x27 */ {';', ':'},
    /* 0x28 */ {'\'', '"'},
    /* 0x29 */ {'`', '~'},
    /* 0x2A */ {shift_l_char, shift_l_char},
    /* 0x2B */ {'\\', '|'},
    /* 0x2C */ {'z', 'Z'},
    /* 0x2D */ {'x', 'X'},
    /* 0x2E */ {'c', 'C'},
    /* 0x2F */ {'v', 'V'},
    /* 0x30 */ {'b', 'B'},
    /* 0x31 */ {'n', 'N'},
    /* 0x32 */ {'m', 'M'},
    /* 0x33 */ {',', '<'},
    /* 0x34 */ {'.', '>'},
    /* 0x35 */ {'/', '?'},
    /* 0x36	*/ {shift_r_char, shift_r_char},
    /* 0x37 */ {'*', '*'},
    /* 0x38 */ {alt_l_char, alt_l_char},
    /* 0x39 */ {' ', ' '},
    /* 0x3A */ {caps_lock_char, caps_lock_char}
    /*其它按键暂不处理*/
};

static void intr_keyboard_handler()
{
    //键盘中断处理程序
    bool ctrl_down_last = ctrl_status;
    bool shift_down_last = shift_status;
    bool alt_down_last = alt_status;

    uint16_t scancode = in_byte(KBD_BUF_PORT); // 读扫描码
    if (scancode == 0xe0)
    {
        //如果是0xe0则向下再读一个扫描码
        ext_scancode = 1;
        return;
    }
    if (ext_scancode == 1)
    {
        scancode = (scancode | 0xe000);
        ext_scancode = 0;
    }
    bool is_breakcode = ((scancode & 0x0080) != 0);
    if (is_breakcode)
    {
        /*是断码的情况 */
        uint16_t make_code = (scancode & 0xff7f);
        if (make_code == ctrl_l_make || make_code == ctrl_r_make)
        {
            ctrl_status = 0;
        }
        else if (make_code == shift_l_make || make_code == shift_r_make)
        {
            shift_status = 0;
        }
        else if (make_code == alt_l_make || make_code == alt_r_make)
        {
            alt_status = 0;
        }
        /*断码的话，如果是控制字符，把对应的status　设置为０即可，别的字符不作处理*/
    }
    else if ((scancode > 0x00 && scancode < 0x3b) ||
             (scancode == alt_r_make) && (scancode == ctrl_l_make))
    {
        bool shift = 0;
        /*是通码的情况*/
        if ((scancode < 0x0e) || (scancode == 0x29) ||
            (scancode == 0x1a) || (scancode == 0x1b) ||
            (scancode == 0x2b) || (scancode == 0x27) ||
            (scancode == 0x28) || (scancode == 0x33) ||
            (scancode == 0x34) || (scancode == 0x35))
        {
            if (shift_down_last == 1)
            {
                shift = 1;
            }
        }
        else
        {
            if (shift_down_last && alt_down_last)
            {
                shift = 0;
            }
            else if (shift_down_last || alt_down_last)
            {
                shift = 1;
            }
            else
            {
                shift = 0;
            }
        }
        //开始输出
        uint8_t index = (scancode & 0x00ff);
        char ch;
        if (shift)
        {
            ch = keymap[index][1];
        }
        else
        {
            ch = keymap[index][0];
        }
        if (ch)
            putChar(ch);
        return;
    }
    else
    {
        putStr("unknow char\n");
    }
}

void keyboard_init()
{
    putStr("keyboard init \n");
    register_handler(0x21, intr_keyboard_handler);
    putStr("keyboard init done \n");
}