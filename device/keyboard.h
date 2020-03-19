#ifndef KEYBOARD_H_
#define KEYBOARD_H_
void keyboard_init();
static void intr_keyboard_handler();
extern struct ioqueue kdb_buf;
#endif