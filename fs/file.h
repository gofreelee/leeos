#ifndef FILE_H_
#define FILE_H_
#include "../lib/stdint.h"
#include "inode.h"
#include "dir.h"
#define MAX_FILE_OPEN 32
struct file
{
    uint32_t fd_pos; // 当前文件操作的偏移地址
    uint32_t fd_flag;
    struct inode *fd_inode;
};

enum std_fd
{
    stdin_no,
    stdout_no,
    stderr_no,
};

enum bitmap_type
{
    INODE_BITMAP,
    BLOCK_BITMAP
};

int32_t get_free_slot_in_global();
int32_t pcb_fd_install(int32_t global_fd_idx);
int32_t inode_bitmap_alloc(struct partition *part);
int32_t block_bitmap_alloc(struct partition *part);
void bitmap_sync(struct partition *part, uint32_t bit_idx, uint8_t btmp);
int32_t file_create(struct dir *parent_dir, char *filename, uint8_t flag);
#endif