#ifndef BITMAP_H_
#define BITMAP_H_
#include "../kernel/global.h"
#include "../lib/stdint.h"
#define BIT_MASK 1

struct bitmap
{
    uint32_t bitmap_len;
    uint8_t *bits;
};

void bitmap_init(struct bitmap *btmp);
uint8_t bitmap_scan_test(struct bitmap *btmp, uint32_t index);
int bitmap_scan(struct bitmap *btmp, uint32_t cnt);
void bitmap_set(struct bitmap *btmp, uint32_t index, uint8_t value);

#endif