#include "bitmap.h"
#include "../lib/string.h"
#include "../kernel/debug.h"
void bitmap_init(struct bitmap *btmp)
{
    memset(btmp->bits, 0, btmp->bitmap_len);
}

uint8_t bitmap_scan_test(struct bitmap *btmp, uint32_t index)
{
    uint32_t base = index / 8;
    uint32_t offset = index % 8;
    return (btmp->bits[base] & (BIT_MASK << offset));
}

int bitmap_scan(struct bitmap *btmp, uint32_t cnt)
{
    // 在bitmap里寻找空闲的,也就是不是为0xff的字节
    uint32_t index = 0;
    while (index < btmp->bitmap_len && (0xff == btmp->bits[index]))
        ++index;
    ASSERT(index < btmp->bitmap_len);
    if (index == btmp->bitmap_len)
    {
        return -1; // 内存池满了
    }

    // 在 btmp->bits[index] 中找到空闲
    int offset = 0;
    while ((BIT_MASK << offset) & btmp->bits[index])
        ++offset;
    int bit_index = index * 8 + offset;

    // 要找连续 cnt 个 空闲的位置
    int bit_left = btmp->bitmap_len * 8 - bit_index; // 剩余的bit
    if (cnt == 1)
        return bit_index;
    int counter = 1;
    bit_index = bit_index + 1;
    while (bit_left > 0)
    {
        if (!bitmap_scan_test(btmp, bit_index))
        {
            ++counter;
            ++bit_index;
            if (counter == cnt)
                return bit_index - (cnt - 1);
        }
        else
        {
            counter = 0;
            ++bit_index;
        }
        --bit_left;
    }
    return -1; //没找到返回 -1
}

void bitmap_set(struct bitmap *btmp, uint32_t index, uint8_t value)
{
    uint32_t base = index / 8;
    uint32_t offset = index % 8;
    if (value)
    {
        btmp->bits[base] = btmp->bits[base] | (BIT_MASK << offset);
    }
    else
    {
        btmp->bits[base] = btmp->bits[base] & ~(BIT_MASK << offset);
    }
}