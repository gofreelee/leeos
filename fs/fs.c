#include "fs.h"
#include "super_block.h"
#include "../device/ide.h"
#include "inode.h"
#include "dir.h"
#include "../lib/kernel/stdio-kernel.h"
#include "../kernel/memory.h"
#include "../lib/string.h"
#include "../kernel/debug.h"
#define DIV_ROUND_UP(X, STEP) (X + STEP - 1) / STEP

struct partition *curr_part;

/*格式化分区．初始化分区的元信息，创建文件系统*/
static void partition_format(struct partition *part)
{
    //1 . 超级快的设置
    uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);
    uint32_t inode_table_sects = DIV_ROUND_UP(sizeof(struct inode) * MAX_FILES_PER_PART, SECTOR_SIZE);
    uint32_t used_sects = 2 + inode_bitmap_sects + inode_table_sects; // 已经使用了的扇区数，引导加超级块（２个）
    uint32_t free_sects = part->sector_counter - used_sects;

    /*处理空闲块 */
    uint32_t block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR);
    uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;
    block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR);
    /*处理空闲块 */

    struct super_block sb;
    sb.magic = 0x19990814;
    sb.sec_cnt = part->sector_counter;
    sb.inode_cnt = MAX_FILES_PER_PART;
    sb.part_lba_base = part->start_lba;
    sb.block_bitmap_lba = part->start_lba + 2; // 跳过引导块和超级块
    sb.block_bitmap_sects = block_bitmap_sects;
    sb.inode_bitmap_lba = sb.block_bitmap_lba + block_bitmap_sects;
    sb.inode_bitmap_sects = inode_bitmap_sects;
    sb.inode_table_lba = sb.inode_bitmap_lba + inode_bitmap_sects;
    sb.inode_table_sects = inode_table_sects;
    sb.data_start_lba = sb.inode_table_sects + inode_table_sects;
    sb.root_inode_no = 0;
    sb.dir_entry_size = sizeof(struct dir_entry);

    printk("%s info:\n", part->name);
    printk("	magic:0x%x\n	part_lba_base:0x%x\n	all_sectors:0x%x\n	inode_cnt:0x%x\n	block_bitmap_lba:0x%x\n	block_bitmap_sectors:0x%x\n	inode_bitmap_lba:0x%x\n	inode_bitmap_sectors:0x%x\n	inode_table_lba:0x%x\n	inode_table_sectors:0x%x\n	data_start_lba:0x%x\n", sb.magic, sb.part_lba_base, sb.sec_cnt, sb.inode_cnt, sb.block_bitmap_lba, sb.block_bitmap_sects, sb.inode_bitmap_lba, sb.inode_bitmap_sects, sb.inode_table_lba, sb.inode_table_sects, sb.data_start_lba);

    //2. 超级块写入磁盘
    struct disk *hd = part->my_disk;
    ide_write(hd, part->start_lba + 1, &sb, 1);
    printk("    super_block_lba:0x%x\n", part->start_lba + 1);

    uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects ? sb.block_bitmap_sects : sb.inode_bitmap_sects);
    buf_size = (buf_size >= sb.inode_table_sects ? buf_size : sb.inode_table_sects) * SECTOR_SIZE;
    uint8_t *buf = sys_malloc(buf_size * 2);
    buf[0] |= 0x01;
    uint32_t block_bitmap_bytes_len = block_bitmap_bit_len / 8;
    uint8_t block_bitmap_odd_idx = block_bitmap_bit_len % 8;
    uint32_t last_size = SECTOR_SIZE - (block_bitmap_bytes_len % SECTOR_SIZE);
    memset(&buf[block_bitmap_bytes_len], 0xff, last_size);
    for (int i = 0; i <= block_bitmap_odd_idx; ++i)
    {
        buf[block_bitmap_odd_idx] &= ~(1 << i);
    }

    ide_write(hd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

    //3. inode位图写入

    memset(buf, 0, buf_size);

    buf[0] != 0x01;
    ide_write(hd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);

    //4. inode table
    memset(buf, 0, buf_size);
    struct inode *inode = (struct inode *)buf;
    inode->i_no = 0;
    inode->i_size = sb.dir_entry_size * 2; // . 和　..
    inode->i_sectors[0] = sb.data_start_lba;
    ide_write(hd, sb.inode_table_lba, buf, sb.inode_table_sects);

    /*5. 根目录写一写 */
    memset(buf, 0, buf_size);
    struct dir_entry *entry = (struct dir_entry *)buf;
    entry->f_type = FT_DIRECTORY;
    entry->i_no = 0;
    memcpy(entry->filename, ".", 1);
    ++entry;

    entry->f_type = FT_DIRECTORY;
    entry->i_no = 0;
    memcpy(entry->filename, "..", 2);
    ide_write(hd, sb.data_start_lba, buf, 1);

    printk(" root_dir_lba: 0x%x\n", sb.data_start_lba);
    printk("%s format done \n", part->name);
    sys_free(buf);
}

static bool mount_partition(struct list_elem *pelem, int arg)
{
    char *name = (char *)arg;
    struct partition *part = get_pcb(struct partition, part_tag, pelem);
    if (!strcmp(part->name, name))
    {
        curr_part = part;
        curr_part->sb = (struct super_block *)sys_malloc(SECTOR_SIZE);
        if (curr_part->sb == 0)
        {
            EXCEPTION_REPORT("alloc memory failed");
        }
        //1. 读超级块
        ide_read(curr_part->my_disk, curr_part->start_lba + 1, curr_part->sb, 1);

        //2.　读块位图
        curr_part->block_bitmap.bits = (uint8_t *)sys_malloc(curr_part->sb->block_bitmap_sects * SECTOR_SIZE);
        if (curr_part->block_bitmap.bits == 0)
        {
            EXCEPTION_REPORT("alloc memory failed");
        }
        curr_part->block_bitmap.bitmap_len = curr_part->sb->block_bitmap_sects * SECTOR_SIZE;
        ide_read(curr_part->my_disk, curr_part->sb->block_bitmap_lba, curr_part->block_bitmap.bits, curr_part->sb->block_bitmap_sects);

        //3. 读inode位图
        curr_part->inode_bitmap.bits = (uint8_t *)sys_malloc(curr_part->sb->inode_bitmap_sects * SECTOR_SIZE);
        if (curr_part->inode_bitmap.bits == 0)
        {
            EXCEPTION_REPORT("alloc memory failed");
        }
        curr_part->inode_bitmap.bitmap_len = curr_part->sb->block_bitmap_sects * SECTOR_SIZE;
        ide_read(curr_part->my_disk, curr_part->sb->inode_bitmap_lba, curr_part->inode_bitmap.bits, curr_part->sb->inode_bitmap_sects);
        list_init(&curr_part->open_inodes);
        printk("mount %s done!\n", part->name);
        return true;
    }
    return false;
}

void filesys_init()
{
    uint32_t channel_idx = 0, dev_idx = 0;
    struct ide_channel *channel;
    struct super_block *sb = (struct super_block *)sys_malloc(sizeof(struct super_block));
    if (sb == 0)
    {
        EXCEPTION_REPORT("alloc memory failed!");
    }
    while (channel_idx < channel_cnt)
    {
        channel = &ide_channels[channel_idx];
        dev_idx = 0;
        while (dev_idx < 2)
        {
            if (dev_idx == 0)
            {
                ++dev_idx;
                continue;
            }
            struct disk *hd = &channel->devices[dev_idx];
            struct partition *part = hd->prim_parts;
            uint32_t part_idx = 0;
            while (part_idx < 12)
            {
                if (part_idx == 4)
                {
                    part = hd->logic_parts;
                }
                if (part->sector_counter != 0)
                {
                    memset(sb, 0, SECTOR_SIZE);
                    ide_read(hd, part->start_lba + 1, sb, 1);
                    if (sb->magic == 0x19990814)
                    {
                        printk("%s has filesystem \n", part->name);
                    }
                    else
                    {
                        printk("formatting %s parttin %s \n", hd->name, part->name);
                        partition_format(part);
                    }
                }
                ++part_idx;
                ++part;
            }
            ++dev_idx;
        }
        ++channel_idx;
    }
    sys_free(sb);
    char default_part[8] = "sdb1";
    list_traveral(&partition_list, mount_partition, (int)default_part);
}

static char *path_parse(char *pathname, char *name_store)
{
    if (pathname[0] == '/')
    {
        while (*(++pathname) == '/')
            ;
    }
    while (*pathname != '/' && *pathname != 0)
    {
        *name_store++ = *pathname++;
    }
    if (pathname[0] == 0)
        return 0;
    return pathname;
}

int32_t path_deepth_cnt(char *pathname)
{
    ASSERT(pathname != 0)
    char *p = pathname;
    char name[MAX_FILE_NAME_LEN];
    uint32_t depth = 0;
    p = path_parse(p, name);
    while (name[0])
    {
        ++depth;
        memset(name, 0, MAX_FILE_NAME_LEN);
        if (p)
        {
            p = path_parse(p, name);
        }
    }
    return depth;
}

static int search_file(const char *filename, struct path_search_record *searched_record)
{
    //先看看是不是根目录
    if (!strcmp(filename, "/") || !strcmp(filename, "/..") || !strcmp(filename, "/."))
    {
        searched_record->parent_dir = &root_dir;
        searched_record->file_type = FT_DIRECTORY;
        searched_record->searched_path[0] = 0;
        return 0;
    }
    char name[MAX_PATH_LEN] = {0};
    char *path_ptr = filename;
    path_ptr = path_parse(path_ptr, name);
    struct dir *parent_dir = &root_dir;
    struct dir_entry p_de;
    searched_record->file_type = FT_UNKNOWN;
    searched_record->parent_dir = parent_dir;
    path_ptr = path_parse(path_ptr, name);
    uint32_t parent_inode_no = 0;
    while (name[0])
    {
        strcat(searched_record->searched_path, "/");
        strcat(searched_record->searched_path, name);

        if (search_dir_entry(curr_part, parent_dir, name, &p_de))
        {
            memset(name, 0, MAX_PATH_LEN);
            if (path_ptr[0])
            {
                path_ptr = path_parse(path_ptr, name);
            }
            if (p_de.f_type == FT_DIRECTORY)
            {
                parent_inode_no = parent_dir->inode->i_no;
                dir_close(parent_dir);
                parent_dir = dir_open(curr_part, p_de.i_no);
                searched_record->parent_dir = parent_dir;
                continue;
            }
            else if (p_de.f_type == FT_REGULAR)
            {
                searched_record->file_type = FT_REGULAR;
                return p_de.i_no;
            }
        }
        else
        {
            return -1;
        }
    }
    //执行到此说明这个要查找的是个目录而不是文件
    dir_close(parent_dir);
    searched_record->parent_dir = dir_open(curr_part, parent_inode_no);
    searched_record->file_type = FT_DIRECTORY;
    return p_de.i_no;
}