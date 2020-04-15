#include "dir.h"
#include "../lib/kernel/stdio-kernel.h"
#include "../lib/string.h"
#include "../kernel/debug.h"
#include "file.h"
struct dir root_dir;

void open_root_dir(struct partition *part)
{
    root_dir.inode = inode_open(part, part->sb->root_inode_no);
    root_dir.dir_pos = 0;
}

struct dir *dir_open(struct partition *part, uint32_t inode_no)
{
    struct dir *pdir = (struct dir *)sys_malloc(sizeof(struct dir));
    pdir->inode = inode_open(part, inode_no);
    pdir->dir_pos = 0;
    return pdir;
}

/*此函数里极有可能有bug */
bool search_dir_entry(struct partition *part, struct dir *pdir,
                      const char *name, struct dir_entry *dir_e)
{
    uint32_t each_inode_sects = 12 + 128; //每个inode最多能占140个扇区
    uint8_t *each_sect_addr_array = (uint8_t *)sys_malloc(12 * 4 + 512);
    if (each_sect_addr_array == 0)
    {
        printk("search dir entry alloc memory failed !!!!");
        return false;
    }
    for (int i = 0; i < 12; ++i)
    {
        each_sect_addr_array[i] = pdir->inode->i_sectors[i];
    }
    /*考虑pdir->inode->i_sectors[12]是否有，来进行下一步读取 */
    if (pdir->inode->i_sectors[12] != 0)
    {
        ide_read(part->my_disk, pdir->inode->i_sectors[12],
                 each_sect_addr_array + 12, 1);
    }
    uint8_t *buf = (uint8_t *)sys_malloc(SECTOR_SIZE);
    struct dir_entry *each_dir_entry = buf;
    uint32_t dir_entry_size = sizeof(struct dir_entry);
    uint32_t dir_entry_cnts = SECTOR_SIZE / dir_entry_size;
    for (int i = 0; i < 140; ++i)
    {
        if (each_sect_addr_array[i] == 0)
            continue;
        ide_read(part->my_disk, each_sect_addr_array[i], buf, 1);
        for (int dir_entry_idx = 0; dir_entry_idx < dir_entry_cnts; ++dir_entry_idx)
        {
            if (!strcmp(each_dir_entry[dir_entry_idx].filename, name))
            {
                memcpy(dir_e, each_dir_entry + dir_entry_idx, dir_entry_size);
                sys_free(buf);
                sys_free(each_sect_addr_array);
                return true;
            }
        }
        memset(buf, 0, SECTOR_SIZE);
    }
    sys_free(buf);
    sys_free(each_sect_addr_array);
    return false;
}

void dir_close(struct dir *dir)
{
    if (dir == &root_dir)
    {
        return;
    }
    inode_close(dir);
    sys_free(dir);
    return;
}

void create_dir_entry(char *filename, uint32_t inode_no,
                      uint8_t file_type, struct dir_entry *p_de)
{
    ASSERT(strlen(filename) <= MAX_FILE_NAME_LEN)
    p_de->i_no = inode_no;
    memcpy(p_de->filename, filename, strlen(filename));
    p_de->f_type = file_type;
}

bool sync_dir_entry(struct dir *parent_dir, struct dir_entry *p_de,
                    void *io_buf)
{
    uint32_t dir_size = parent_dir->inode->i_size; //
    uint32_t dir_cnts_per_sec = SECTOR_SIZE / sizeof(struct dir);
    ASSERT((dir_size % sizeof(struct dir)) == 0);
    struct inode *dir_inode = parent_dir->inode;
    uint32_t dir_entry_cnts = SECTOR_SIZE / sizeof(struct dir_entry);
    struct dir_entry *dir_e;
    uint32_t block_bitmap_idx = -1;
    uint32_t block_lba;
    uint32_t all_blocks[12 + 128] = {0};
    for (int i = 0; i < 12; ++i)
    {
        all_blocks[i] = dir_inode->i_sectors[i];
    }
    // 查看dir_inode->i_sectors[12]是否是０
    if (dir_inode->i_sectors[12] != 0)
    {
        block_lba = dir_inode->i_sectors[12];
        ide_read(curr_part->my_disk, block_lba, all_blocks + 12, 1);
    }
    for (int block_idx = 0; block_idx < 140; ++block_idx)
    {
        if (all_blocks[block_idx] == 0)
        {
            block_lba = block_bitmap_alloc(curr_part);
            if (block_lba == -1)
            {
                printk("alloc block bitmap for sync_dir_entry failed\n");
                return false;
            }
            block_bitmap_idx = block_lba - curr_part->sb->data_start_lba;
            bitmap_sync(curr_part, block_bitmap_idx, BLOCK_BITMAP);
            if (block_idx < 12)
            {
                dir_inode->i_sectors[block_idx] = all_blocks[block_idx] = block_lba;
            }
            else if (block_idx == 12)
            {
                /* code */
                dir_inode->i_sectors[block_idx] = block_lba;
                block_lba = block_bitmap_alloc(curr_part);
                if (block_lba == -1)
                {
                    block_lba = dir_inode->i_sectors[block_idx];
                    block_bitmap_idx = block_lba - curr_part->sb->data_start_lba;
                    bitmap_set(&curr_part->block_bitmap, block_bitmap_idx, 0);
                    dir_inode->i_sectors[block_idx] = 0;
                    return false;
                }
                block_bitmap_idx = block_lba - curr_part->sb->data_start_lba;
                all_blocks[block_idx] = block_lba;
                bitmap_sync(curr_part, block_bitmap_idx, BLOCK_BITMAP);
                ide_write(curr_part->my_disk, dir_inode->i_sectors[block_idx], all_blocks + 12, 1);
            }
            else
            {
                block_lba = block_bitmap_alloc(curr_part);
                block_bitmap_idx = block_lba - curr_part->sb->data_start_lba;
                if (block_lba == -1)
                {
                }
                bitmap_sync(curr_part, block_bitmap_idx, BLOCK_BITMAP);
                all_blocks[block_idx] = block_lba;
                ide_write(curr_part->my_disk, dir_inode->i_sectors[12], all_blocks + 12, 1);
            }
            memset(io_buf, 0, 512);
            memcpy(io_buf, p_de, sizeof(struct dir_entry));
            ide_write(curr_part->my_disk, all_blocks[block_idx], io_buf, 1);
            dir_inode->i_size += sizeof(struct dir_entry);
            return true;
        }

        //此扇区已经使用,寻找一下空位
        ide_read(curr_part->my_disk, all_blocks[block_idx], io_buf, 1);
        dir_e = (struct dir_entry *)io_buf;
        for (int dir_entry_idx = 0; dir_entry_idx < dir_entry_cnts; ++dir_entry_idx)
        {
            if ((dir_e + dir_entry_idx)->f_type == FT_UNKNOWN)
            {
                //找到空余的
                memcpy(dir_e + dir_entry_idx, p_de, sizeof(struct dir_entry));
                ide_write(curr_part->my_disk, all_blocks[block_idx], io_buf, 1);
                dir_inode->i_open_cnts += sizeof(struct dir_entry);
                return true;
            }
        }
    }
}