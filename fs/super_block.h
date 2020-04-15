#ifndef SUPER_BLOCK_H_
#define SUPER_BLOCK_H_
#include "../lib/stdint.h"

/*超级块数据结构 */
struct super_block
{
    uint32_t magic;         //用来区分文件系统的类型
    uint32_t sec_cnt;       // 该分区的扇区总数
    uint32_t inode_cnt;     //inode的数量，(也是能存储的最大文件数?)
    uint32_t part_lba_base; // 分区的起始lba地址

    uint32_t block_bitmap_lba;   // 块位图起始扇区lba地址
    uint32_t block_bitmap_sects; //　块位图占用扇区数量

    uint32_t inode_bitmap_lba;   // inode位图．．．．．
    uint32_t inode_bitmap_sects; // inode 位图占用的扇区数量

    uint32_t inode_table_lba;   // inode　节点表的lba地址
    uint32_t inode_table_sects; //  inode 节点表的占用扇区

    uint32_t data_start_lba; //数据区开始的第一个扇区号
    uint32_t root_inode_no;  // 根目录对应的inode节点在inode节点表里的下标
    uint32_t dir_entry_size; // 目录项的大小

    uint8_t pad[460];
} __attribute__ ((packed));

#endif