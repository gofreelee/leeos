#ifndef INODE_H_
#define INODE_H_
#include "../lib/stdint.h"
#include "../lib/kernel/list.h"
#include "../device/ide.h"
struct inode
{
    uint32_t i_no;              //inode编号
    uint32_t i_size;            //当inode是文件时，表示文件的大小，当inode是目录时表示目录下的目录项之和
    uint32_t i_open_cnts;       // 记录文件被打开的次数
    bool write_deny;            //写文件不能并行，写文件之前要检查此标识
    uint32_t i_sectors[13];     // 0 - 11是直接块，12是存储一级间接指针的
    struct list_elem inode_tag; // 用于加入已打开的inode队列
};

struct inode_position
{
    bool two_sec;
    uint32_t sec_lba; //
    uint32_t off_size;
};

static void inode_locate(struct partition *part, uint32_t inode_no,
                         struct inode_position *inode_pos);

void inode_sync(struct partition *part, struct inode *inode, void *io_buff);
struct inode *inode_open(struct partition *part, uint32_t inode_no);
struct inode *inode_close(struct inode *inode);
void inode_init(uint32_t inode_no, struct inode *new_inode);
#endif