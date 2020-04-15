#ifndef IDE_H_
#define IDE_H_
#include "../lib/stdint.h"
#include "../lib/kernel/list.h"
#include "../lib/bitmap.h"
#include "../thread/sync.h"
#include "../fs/super_block.h"
/*此文件定义很多硬盘相关的数据结构*/

/*分区结构 */
struct partition
{
    uint32_t start_lba;         //起始扇区
    uint32_t sector_counter;    //扇区数量
    struct disk *my_disk;       //分区所属硬盘
    struct list_elem part_tag;  //用于拉成链表
    char name[8];               //分区名称
    struct super_block *sb;     //本分区的超级块
    struct bitmap block_bitmap; //块位图
    struct bitmap inode_bitmap; //inode　位图
    struct list open_inodes;    // 本分区打开的inode节点队列
};

/*硬盘结构*/
struct disk
{
    char name[8];                    //硬盘名称
    struct ide_channel *my_channel;  //硬盘属于哪个IDE通道
    uint8_t dev_no;                  //0表示主盘　　１表示从盘
    struct partition prim_parts[4];  // 最多四个主分区
    struct partition logic_parts[8]; // 人为设定逻辑分区最多８个
};

/*ata通道结构 */
struct ide_channel
{
    char name[8];                   //ata通道的名称
    uint16_t port_base;             //　通道的起始端口号
    uint8_t interrupt_number;       // 本通道的对应中断号
    struct lock lock;               //通道锁
    bool expection_interrupt;       //是否在等待硬盘的中断
    struct semaphore disk_done_sem; //阻塞与唤醒驱动程序
    struct disk devices[2];         // 一个通道对应两个硬盘　一主一从
};
extern uint8_t channel_cnt;
extern struct ide_channel ide_channels[2];
extern struct list partition_list;
void ide_init();
static select_disk(struct disk *hd);                                       // 选择要读写的硬盘,主盘还是从盘
static void select_sector(struct disk *hd, uint32_t lba, uint8_t sec_cnt); //　起始扇区地址和读写扇区数
static void cmd_out(struct ide_channel *channel, uint8_t cmd);
static void read_from_sector(struct disk *hd, void *buf, uint8_t sec_cnt);
static void write_to_sector(struct disk *hd, void *buf, uint8_t sec_cnt);
static bool busy_wait(struct disk *hd);
void ide_read(struct disk *hd, uint32_t lba, void *buf, uint32_t sec_cnt);
void ide_write(struct disk *hd, uint32_t lba, void *buf, uint32_t sec_cnt);
void intr_hd_handler(uint8_t irq_no); //硬盘中断处理程序
static bool partition_info(struct list_elem *pelem, int arg UNUSED);
static void partition_scan(struct disk *hd, uint32_t ext_lba);
static void identify_disk(struct disk *hd);

#endif