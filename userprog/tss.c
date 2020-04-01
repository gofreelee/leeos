#include "../lib/stdint.h"
#include "../thread/thread.h"
#include "../kernel/global.h"
#include "../lib/kernel/print.h"
#include "../lib/string.h"
struct tss
{
    uint32_t backlink;
    uint32_t *esp0;
    uint32_t *ss0;
    uint32_t *esp1;
    uint32_t *ss1;
    uint32_t *esp2;
    uint32_t *ss2;
    uint32_t cr3;
    uint32_t (*eip)(void);
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trace;
    uint32_t io_base;
};
static struct tss tss;

/*设置tss的esp0指针． */
void update_tss_esp0(struct pcb_struct *pthread)
{
    tss.esp0 = (uint32_t *)((uint32_t)pthread + 4096);
}

/*创建gdt描述符 */
static struct gdt_desc make_gdt_desc(uint32_t *desc_addr, uint32_t limit,
                                     uint8_t attr_low, uint8_t attr_high)
{

    struct gdt_desc gdt_desciption;
    uint32_t base_addr = (uint32_t)desc_addr;
    gdt_desciption.base_low_word = (base_addr & 0x0000ffff);
    gdt_desciption.base_mid_byte = ((base_addr & 0x00ff0000) >> 16);
    gdt_desciption.base_high_byte = ((base_addr & 0xff000000) >> 24);
    gdt_desciption.limit_low_word = (limit & 0x0000ffff);
    gdt_desciption.attr_low_byte = attr_low;
    gdt_desciption.limit_high_attr_high = attr_high + ((limit & 0x000f0000) >> 16);

    return gdt_desciption;
}

void tss_init()
{
    putStr("tss init start \n");
    uint32_t tss_size = sizeof(struct tss);
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = SELECTOR_K_STACK;
    tss.io_base = tss_size;

    *((struct gdt_desc *)0xc0000923) =
        make_gdt_desc((uint32_t *)&tss, tss_size - 1, TSS_ATTR_LOW, TSS_ATTR_HIGH);

    *((struct gdt_desc *)0xc000092b) =
        make_gdt_desc((uint32_t *)0, 0xfffff, GDT_ATTR_CODE_LOW_DPL_3, GDT_ATTR_HIGH);
    *((struct gdt_desc *)0xc0000933) =
        make_gdt_desc((uint32_t *)0, 0xfffff, GDT_ATTR_DATA_LOW_DPL_3, GDT_ATTR_HIGH);

    uint64_t gdt_operand =
        (((uint64_t)(uint32_t)0xc0000903) << 16) | (8 * 7 - 1);
    asm volatile("lgdt %0" ::"m"(gdt_operand));

    asm volatile("ltr  %w0" ::"r"(SELECTOR_TSS));

    putStr("tss init done \n");
}