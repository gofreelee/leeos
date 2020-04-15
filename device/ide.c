#include "ide.h"
#include "../lib/kernel/print.h"
#include "../lib/stdio.h"
#include "../kernel/debug.h"
#include "../kernel/io.h"
#include "timer.h"
#include "../kernel/interrupt.h"
#include "../lib/kernel/stdio-kernel.h"
#include "../lib/string.h"

/*定义硬盘驱动器的端口号*/
#define reg_data(channel) (channel->port_base + 0)
#define reg_error(channel) (channel->port_base + 1)
#define reg_sector_count(channel) (channel->port_base + 2)
#define reg_lba_l(channel) (channel->port_base + 3)
#define reg_lba_m(channel) (channel->port_base + 4)
#define reg_lba_h(channel) (channel->port_base + 5)
#define reg_dev(channel) (channel->port_base + 6)
#define reg_status(channel) (channel->port_base + 7)
#define reg_cmd(channel) reg_status(channel)
#define reg_alt_status(channel) (channel->port_base + 0x206)
#define reg_ctl(channel) reg_alt_status(channel)

/*status寄存器的一些关键位*/
#define BIT_ALT_STAT_BUSY 0x80         //硬盘正忙
#define BIT_ALT_STAT_DEVICE_READY 0x40 //设备就绪
#define BIT_ALT_STAT_DATA_READY 0x8    //数据准备就绪

/*device寄存器的一些关键位*/
#define BIT_DEV_DEFAULT 0xa0 //第七位和第五位默认1
#define BIT_DEV_LBA 0x40     //寻址模式LBA
#define BIT_DEV_DEV 0X10     //主盘

/*硬盘操作命令*/
#define CMD_IDENTIFY 0xec
#define CMD_READ_SECTOR 0x20
#define CMD_WRITE_SECTOR 0x30

#define max_lba ((80 * 1024 * 1024 / 512) - 1) // 最大扇区数80MB
 #define DIV_ROUND_UP(X, STEP) (X + STEP - 1) / STEP
uint8_t channel_cnt;                //通道数
struct ide_channel ide_channels[2]; // 固定两个IDE通道

int32_t extra_lba_base = 0; // 用于记录总拓展分区的起始

uint8_t primary_idx = 0, logic_idx = 0; // 硬盘主分区和逻辑分区的下标

struct list partition_list; // 用来连逻辑分区的列表

// /*16字节，分区表项 */
struct partition_table_entry
{
    uint8_t bootable;   //是否可引导
    uint8_t start_head; // 起始磁头号
    uint8_t start_sec;  // 起始扇区号
    uint8_t start_chs;  // 起始柱面号
    uint8_t fs_type;    // 分区类型
    uint8_t end_head;   // 结束磁头号
    uint8_t end_sec;    // 结束扇区号
    uint8_t end_chs;    // 结束柱面号

    uint32_t start_lba; // 本分区起始扇区的lba地址
    uint32_t sec_cnt;   // 本分区的地址
} __attribute__((packed));

// /*引导扇区*/
struct boot_sector
{
    uint8_t othter[446]; // 引导代码
    struct partition_table_entry partition_table[4];
    uint16_t signature;
} __attribute__((packed));

void ide_init()
{
    printk("ide_init start\n");
    uint8_t hd_cnt = *((uint8_t *)(0x475)); // 获取硬盘的数量
    printk("   ide_init hd_cnt:%d\n", hd_cnt);
    ASSERT(hd_cnt > 0);
    list_init(&partition_list);
    channel_cnt = DIV_ROUND_UP(hd_cnt, 2); // 一个ide通道上有两个硬盘,根据硬盘数量反推有几个ide通道
    struct ide_channel *channel;
    uint8_t channelogic_idx = 0, dev_no = 0;
    putInt(channel_cnt);
    /* 处理每个通道上的硬盘 */
    while (channelogic_idx < channel_cnt)
    {
        channel = &ide_channels[channelogic_idx];
        sprintf(channel->name, "ide%d", channelogic_idx);

        /* 为每个ide通道初始化端口基址及中断向量 */
        switch (channelogic_idx)
        {
        case 0:
            channel->port_base = 0x1f0;            // ide0通道的起始端口号是0x1f0
            channel->interrupt_number = 0x20 + 14; // 从片8259a上倒数第二的中断引脚,温盘,也就是ide0通道的的中断向量号
            break;
        case 1:
            channel->port_base = 0x170;            // ide1通道的起始端口号是0x170
            channel->interrupt_number = 0x20 + 15; // 从8259A上的最后一个中断引脚,我们用来响应ide1通道上的硬盘中断
            break;
        }

        channel->expection_interrupt = false; // 未向硬盘写入指令时不期待硬盘的中断
        lock_init(&channel->lock);
        /* 初始化为0,目的是向硬盘控制器请求数据后,硬盘驱动sem_down此信号量会阻塞线程,直到硬盘完成后通过发中断,由中断处理程序将此信号量sema_up,唤醒线程. */
        sem_init(&channel->disk_done_sem, 0);

        register_handler(channel->interrupt_number, intr_hd_handler);
        /* 分别获取两个硬盘的参数及分区信息 */
        while (dev_no < 2)
        {
            struct disk *hd = &channel->devices[dev_no];
            hd->my_channel = channel;
            hd->dev_no = dev_no;
            sprintf(hd->name, "sd%c", 'a' + channelogic_idx * 2 + dev_no);
            identify_disk(hd); // 获取硬盘参数
            if (dev_no != 0)
            {                          // 内核本身的裸硬盘(hd60M.img)不处理
                partition_scan(hd, 0); // 扫描该硬盘上的分区
            }
            primary_idx = 0, logic_idx = 0;
            dev_no++;
        }
        dev_no = 0;        // 将硬盘驱动器号置0,为下一个channel的两个硬盘初始化。
        channelogic_idx++; // 下一个channel
    }

    printk("\n   all partition info\n");
    /* 打印所有分区信息 */
    list_traveral(&partition_list, partition_info, 0);
    printk("ide_init done\n");
}

static select_disk(struct disk *hd)
{
    uint8_t reg_device = BIT_DEV_LBA | BIT_DEV_DEFAULT;
    if (hd->dev_no == 1)
    {
        reg_device |= BIT_DEV_DEV; // 设置主盘
    }
    out_byte(reg_dev(hd->my_channel), reg_device);
}

static void select_sector(struct disk *hd, uint32_t lba, uint8_t sec_cnt)
{
    ASSERT(lba <= max_lba);
    out_byte(reg_sector_count(hd->my_channel), sec_cnt);
    out_byte(reg_lba_l(hd->my_channel), lba);
    out_byte(reg_lba_m(hd->my_channel), lba >> 8);
    out_byte(reg_lba_h(hd->my_channel), lba >> 16);

    uint8_t last_8_bits = BIT_DEV_DEFAULT | BIT_DEV_LBA | (hd->dev_no == 1 ? BIT_DEV_DEV : 0) | (lba >> 24);
    out_byte(reg_dev(hd->my_channel), last_8_bits);

} //　起始扇区地址和读写扇区数

static void cmd_out(struct ide_channel *channel, uint8_t cmd)
{
    ASSERT(channel != 0)
    channel->expection_interrupt = 1;
    out_byte(reg_cmd(channel), cmd);
}

static void read_from_sector(struct disk *hd, void *buf, uint8_t sec_cnt)
{
    uint32_t bytes_cnt;
    if (sec_cnt == 0)
    {
        bytes_cnt = 256 * 512;
    }
    else
    {
        bytes_cnt = sec_cnt * 512;
    }
    in_multibyte(reg_data(hd->my_channel), buf, bytes_cnt / 2);
}

static void write_to_sector(struct disk *hd, void *buf, uint8_t sec_cnt)
{
    uint32_t bytes_cnt;
    if (sec_cnt == 0)
    {
        bytes_cnt = 256 * 512;
    }
    else
    {
        bytes_cnt = sec_cnt * 512;
    }
    out_multibyte(reg_data(hd->my_channel), buf, bytes_cnt / 2);
}

static bool busy_wait(struct disk *hd)
{
    struct ide_channel *channel = hd->my_channel;
    uint16_t time_limit = 30 * 1000; // 可以等待30000毫秒
    while (time_limit -= 10 >= 0)
    {
        if (!(in_byte(reg_status(channel)) & BIT_ALT_STAT_BUSY))
        {

            return (in_byte(reg_status(channel)) & BIT_ALT_STAT_DATA_READY);
        }
        else
        {
            mtime_sleep(10); // 睡眠10毫秒
        }
    }
    return false;
}

void ide_read(struct disk *hd, uint32_t lba, void *buf, uint32_t sec_cnt)
{
    ASSERT(lba <= max_lba)
    ASSERT(sec_cnt > 0)
    lock_acquire(&hd->my_channel->lock);
    uint32_t sec_op;
    uint32_t sec_done = 0;
    select_disk(hd); // 1 确定磁盘

    while (sec_done < sec_cnt)
    {
        if ((sec_cnt - sec_done) >= 256)
        {
            sec_op = 256;
        }
        else
        {
            sec_op = sec_cnt - sec_done;
        }
        select_sector(hd, lba + sec_done, sec_op); // 2 设定扇区和读的扇区数
        cmd_out(hd->my_channel, CMD_READ_SECTOR);
        sem_down(&(hd->my_channel->disk_done_sem)); // 阻塞等磁盘
        if (!busy_wait(hd))
        {
            /*出错了 */
            char error[64];
            sprintf(error, "%s read sector %d failed!!!!!\n", hd->name, lba);
            EXCEPTION_REPORT(error);
        }
        read_from_sector(hd, (void *)((uint32_t)buf + 512 * sec_done), sec_op);
        sec_done += sec_op;
    }
    lock_release(&(hd->my_channel->lock));
}

void ide_write(struct disk *hd, uint32_t lba, void *buf, uint32_t sec_cnt)
{
    ASSERT(lba <= max_lba)
    ASSERT(sec_cnt > 0)
    lock_acquire(&(hd->my_channel->lock));
    uint32_t sec_op;
    uint32_t sec_done = 0;
    select_disk(hd); // 1 确定磁盘

    while (sec_done < sec_cnt)
    {
        if ((sec_cnt - sec_done) >= 256)
        {
            sec_op = 256;
        }
        else
        {
            sec_op = sec_cnt - sec_done;
        }
        select_sector(hd, lba + sec_done, sec_op); // 2 设定扇区和读的扇区数
        cmd_out(hd->my_channel, CMD_WRITE_SECTOR);

        if (!busy_wait(hd))
        {
            /*出错了 */
            char error[64];
            sprintf(error, "%s write sector %d failed!!!!!\n", hd->name, lba);
            EXCEPTION_REPORT(error);
        }
        write_to_sector(hd, (void *)((uint32_t)buf + 512 * sec_done), sec_op);
        sem_down(&(hd->my_channel->disk_done_sem)); // 阻塞等磁盘

        sec_done += sec_op;
    }
    lock_release(&(hd->my_channel->lock));
}

static void swap_pairs_bytes(const char *dst, char *buf, uint32_t len)
{
    uint8_t idx;
    for (idx = 0; idx < len; idx += 2)
    {
        buf[idx + 1] = *dst++;
        buf[idx] = *dst++;
    }
    buf[idx] = '\0';
}

static void identify_disk(struct disk *hd)
{
    char hd_info[512];
    select_disk(hd);
    cmd_out(hd->my_channel, CMD_IDENTIFY);
    sem_down(&hd->my_channel->disk_done_sem);
    if (!busy_wait(hd))
    {
        char error[64];
        sprintf(error, "%s identify failed!!!!!\n", hd->name);
        EXCEPTION_REPORT(error);
    }

    read_from_sector(hd, hd_info, 1);
    char buf[64];
    uint8_t sn_start = 10 * 2, sn_len = 20, md_start = 27 * 2, md_len = 40;

    swap_pairs_bytes(&hd_info[sn_start], buf, sn_len);

    printk(" disk %s info: \n  SN: %s\n", hd->name, buf);
    memset(buf, 0, sizeof(buf));
    swap_pairs_bytes(&hd_info[md_start], buf, md_len);
    printk("  MODULE: %s\n", buf);
    uint32_t sectors = *(uint32_t *)&hd_info[60 * 2];
    printk("  SECTORS: %d\n", sectors);
    printk("  CAPACITY: %d\n", sectors * 512 / 1024 / 1024);
}

void intr_hd_handler(uint8_t irq_no)
{
    ASSERT(irq_no == 0x2e || irq_no == 0x2f)
    uint8_t index = irq_no - 0x2e;
    struct ide_channel *channel = &ide_channels[index];
    if (ide_channels[index].expection_interrupt == 1)
    {
        ide_channels[index].expection_interrupt = 0;
        sem_up(&ide_channels[index].disk_done_sem);
        in_byte(reg_status(channel));
    }
}

// static void partition_scan(struct disk *hd, uint32_t ext_lba)
// {
//     struct boot_sector *mbr = sys_malloc(sizeof(struct boot_sector));
//     ide_read(hd, ext_lba, mbr, 1);
//     struct partition_table_entry *partition_table = mbr->partition_table;
//     putStr("\n");
//     putInt((partition_table + 1)->fs_type);
//     int counter = 0;
//     while (counter< 4)
//     {
//         if ((partition_table + counter)->fs_type == 0x05)
//         {
//             if (ext_lba == 0)
//             {
//                 extra_lba_base = (partition_table + counter)->start_lba;
//                 partition_scan(hd, extra_lba_base);
//             }
//             else
//             {
//                 partition_scan(hd, extra_lba_base + (partition_table + counter)->start_lba);
//             }
//         }
//         else if ((partition_table + counter)->fs_type != 0)
//         {
//             if (ext_lba == 0)
//             {
//                 putStr("debug");
//                 /*主分区*/
//                 hd->prim_parts[primary_idx].start_lba = ext_lba + (partition_table + counter)->start_lba;
//                 hd->prim_parts[primary_idx].my_disk = hd;
//                 hd->prim_parts[primary_idx].sector_counter = (partition_table + counter)->sec_cnt;
//                 list_append(&partition_list, &hd->prim_parts[primary_idx].part_tag);
//                 sprintf(hd->prim_parts[primary_idx].name, "%s%d", hd->name, primary_idx + 1);
//                 ++primary_idx;
//                 ASSERT(primary_idx < 4)
//             }
//             else
//             {
//                 /*逻辑分区 */
//                 hd->logic_parts[logic_idx].start_lba = ext_lba + (partition_table + counter)->start_lba;
//                 hd->logic_parts[logic_idx].my_disk = hd;
//                 hd->logic_parts[logic_idx].sector_counter = (partition_table + counter)->sec_cnt;
//                 list_append(&partition_list, &hd->logic_parts[logic_idx].part_tag);
//                 sprintf(hd->logic_parts[logic_idx].name, "%s%d", hd->name, logic_idx + 5);
//                 ++logic_idx;
//                 if (logic_idx >= 8)
//                     return;
//             }
//         }
//         else
//         {
//             putStr("wtf?");
//         }
//         ++counter;
//     }
//     sys_free(mbr);
// }

static void partition_scan(struct disk *hd, uint32_t ext_lba)
{
    struct boot_sector *bs = sys_malloc(sizeof(struct boot_sector));
    ide_read(hd, ext_lba, bs, 1);
    uint8_t part_idx = 0;
    struct partition_table_entry *p = bs->partition_table;

    /* 遍历分区表4个分区表项 */
    while (part_idx++ < 4)
    {
        if (p->fs_type == 0x5)
        { // 若为扩展分区
            if (extra_lba_base != 0)
            {
                /* 子扩展分区的start_lba是相对于主引导扇区中的总扩展分区地址 */
                partition_scan(hd, p->start_lba + extra_lba_base);
            }
            else
            { // ext_lba_base为0表示是第一次读取引导块,也就是主引导记录所在的扇区
                /* 记录下扩展分区的起始lba地址,后面所有的扩展分区地址都相对于此 */
                extra_lba_base = p->start_lba;
                partition_scan(hd, p->start_lba);
            }
        }
        else if (p->fs_type != 0)
        { // 若是有效的分区类型
            if (ext_lba == 0)
            { // 此时全是主分区
                hd->prim_parts[primary_idx].start_lba = ext_lba + p->start_lba;
                hd->prim_parts[primary_idx].sector_counter = p->sec_cnt;
                hd->prim_parts[primary_idx].my_disk = hd;
                list_append(&partition_list, &hd->prim_parts[primary_idx].part_tag);
                sprintf(hd->prim_parts[primary_idx].name, "%s%d", hd->name, primary_idx + 1);
                primary_idx++;
                ASSERT(primary_idx < 4); // 0,1,2,3
            }
            else
            {
                hd->logic_parts[logic_idx].start_lba = ext_lba + p->start_lba;
                hd->logic_parts[logic_idx].sector_counter = p->sec_cnt;
                hd->logic_parts[logic_idx].my_disk = hd;
                list_append(&partition_list, &hd->logic_parts[logic_idx].part_tag);
                sprintf(hd->logic_parts[logic_idx].name, "%s%d", hd->name, logic_idx + 5); // 逻辑分区数字是从5开始,主分区是1～4.
                logic_idx++;

                // 只支持8个逻辑分区,避免数组越界
                if (logic_idx >= 8)
                {
                    return;
                }
            }
        }
        p++;
    }
    sys_free(bs);
}

/*打印分区信息*/
static bool partition_info(struct list_elem *pelem, int arg UNUSED)
{
    struct partition *part = get_pcb(struct partition, part_tag, pelem);
    printk("  %s start_lba: 0x%x, sec_cnt: 0x%x\n", part->name, part->start_lba, part->sector_counter);
    return false;
}
