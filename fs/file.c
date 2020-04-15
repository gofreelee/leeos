#include "file.h"
#include "../lib/kernel/stdio-kernel.h"
#include "fs.h"
struct file file_table[MAX_FILE_OPEN];

int32_t get_free_slot_in_global()
{
    int idx;
    for (idx = 3; idx < MAX_FILE_OPEN; ++idx)
    {
        if (file_table[idx].fd_inode == 0)
            break;
    }
    if (idx == MAX_FILE_OPEN)
    {
        printk("exceed max open files\n");
        return -1;
    }
    return idx;
}

int32_t pcb_fd_install(int32_t global_fd_idx)
{
    int idx;
    struct pcb_struct *curr_prog = running_thread();
    for (idx = 3; idx < MAX_FILES_OPEN_PER_PROC; ++idx)
    {
        if (curr_prog->fd_table[idx] == -1)
        {
            curr_prog->fd_table[idx] = global_fd_idx;
            break;
        }
    }
    if (idx == MAX_FILES_OPEN_PER_PROC)
    {
        printk("exceed max open file_per_proc\n");
        return -1;
    }
    return idx;
}

/*分配一个inode 返回节点号 */
int32_t inode_bitmap_alloc(struct partition *part)
{
    int idx = bitmap_scan(&part->inode_bitmap, 1);
    if (idx == -1)
    {
        return -1;
    }
    bitmap_set(&part->inode_bitmap, idx, 1);
    return idx;
}

/*分配一个扇区(块),并返回扇区地址 */
int32_t block_bitmap_alloc(struct partition *part)
{
    int idx = bitmap_scan(&part->block_bitmap, 1);
    if (idx == -1)
    {
        return -1;
    }
    bitmap_set(&part->block_bitmap, idx, 1);
    return part->sb->data_start_lba + idx;
}

void bitmap_sync(struct partition *part, uint32_t bit_idx, uint8_t btmp)
{
    uint32_t off_sec = bit_idx / (SECTOR_SIZE * 8);
    uint32_t off_size = BLOCK_SIZE * off_sec;
    uint32_t sec_lba;
    uint8_t *bitmap_off;
    if (btmp == INODE_BITMAP)
    {
        sec_lba = part->sb->inode_table_lba + off_sec;
        bitmap_off = part->inode_bitmap.bits + off_size;
    }
    else
    {
        sec_lba = part->sb->block_bitmap_lba + off_sec;
        bitmap_off = part->inode_bitmap.bits + off_size;
    }
    ide_write(part->my_disk, sec_lba, bitmap_off, 1);
}

int32_t file_create(struct dir *parent_dir, char *filename, uint8_t flag)
{
    
}