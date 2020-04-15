#include "inode.h"
#include "../kernel/debug.h"
#include "../lib/string.h"
#include "../kernel/interrupt.h"

static void inode_locate(struct partition *part, uint32_t inode_no,
                         struct inode_position *inode_pos)
{
    ASSERT(inode_no < 4096)
    uint32_t inode_size = sizeof(struct inode);
    uint32_t inode_table_lba = part->sb->inode_table_lba;
    uint32_t inode_sum_size = inode_no * inode_size;
    uint32_t the_inode_start_lba = inode_table_lba + (inode_sum_size / 512);
    uint32_t left_inode_size = inode_sum_size % 512;
    if (512 - left_inode_size > inode_size)
    {
        inode_pos->two_sec = false;
    }
    else
    {
        inode_pos->two_sec = true;
    }
    inode_pos->sec_lba = the_inode_start_lba;
    inode_pos->off_size = left_inode_size;
}

void inode_sync(struct partition *part, struct inode *inode, void *io_buff)
{
    /*函数功能，把inode的数据写到磁盘 */

    struct inode_position inode_pos;
    inode_locate(part, inode->i_no, &inode_pos);
    ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sector_counter))
    struct inode pure_inode;
    memcpy(&pure_inode, inode, sizeof(struct inode));
    pure_inode.i_open_cnts = 0;
    pure_inode.write_deny = false;
    pure_inode.inode_tag.next = 0;
    pure_inode.inode_tag.prev = 0;
    char *inode_buf = (char *)io_buff;
    if (inode_pos.two_sec)
    {
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
        memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    }
    else
    {
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
        memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
    }
}

struct inode *inode_open(struct partition *part, uint32_t inode_no)
{
    /*先在分区的openlist中寻找 */
    struct list_elem *tmp_elem;
    struct inode *curr_inode;
    tmp_elem = part->open_inodes.head.next;
    /*在已有的缓存队列康一康 */
    while (tmp_elem != &part->open_inodes.tail)
    {
        curr_inode = get_pcb(struct inode, inode_tag, tmp_elem);
        if (curr_inode->i_no == inode_no)
        {
            curr_inode->i_open_cnts++;
            return curr_inode; // 找到返回
        }
        tmp_elem = tmp_elem->next;
    }
    /*＂欺骗＂程序，不论是用户进程还是内核进程，让他们从内核中分配 */
    struct pcb_struct *curr_prog = running_thread();
    uint32_t *keep_pgdir = curr_prog->pgdir;
    curr_prog->pgdir = 0;
    curr_inode = (struct inode *)sys_malloc(sizeof(struct inode));
    curr_prog->pgdir = keep_pgdir;

    char *io_buf;
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);
    if (inode_pos.two_sec)
    {
        io_buf = sys_malloc(1024); // 两个扇区
        ide_read(part->my_disk, inode_pos.sec_lba, io_buf, 2);
    }
    else
    {
        io_buf = sys_malloc(512); //一个扇区
        ide_read(part->my_disk, inode_pos.sec_lba, io_buf, 1);
    }
    memcpy(curr_inode, io_buf + inode_pos.off_size, sizeof(struct inode));
    list_append(&part->open_inodes, &curr_inode->inode_tag);
    curr_inode->i_open_cnts = 1;
    sys_free(io_buf);
    return curr_inode;
}

struct inode *inode_close(struct inode *inode)
{
    enum intr_status old_status = intr_disable();
    --inode->i_open_cnts;
    if (inode->i_open_cnts == 0)
    {
        list_remove(&inode->inode_tag);
        struct pcb_struct *curr_prog = running_thread();
        uint32_t *keep_pgdir = curr_prog->pgdir;
        curr_prog->pgdir = 0;
        sys_free(inode);
        curr_prog->pgdir = keep_pgdir;
    }
    intr_set_status(old_status);
}

void inode_init(uint32_t inode_no, struct inode *new_inode)
{
    new_inode->i_no = inode_no;
    new_inode->i_open_cnts = 0;
    new_inode->i_size = 0;
    new_inode->write_deny = false;
    for (int sec_idx = 0; sec_idx < 13; ++sec_idx)
    {
        new_inode->i_sectors[sec_idx] = 0;
    }
}