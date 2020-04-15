#ifndef FS_H_
#define FS_H_
#include "../device/ide.h"
#define MAX_FILES_PER_PART 4096 // 每个分区支持创建的最大文件数
#define BITS_PER_SECTOR 4096    // 每个扇区的位数
#define SECTOR_SIZE 512         // 扇区字节大小
#define BLOCK_SIZE SECTOR_SIZE  // 块大小，就是一个扇区的大小
#define MAX_FILE_NAME_LEN 16
#define MAX_PATH_LEN 512
/*文件类型*/
enum file_types
{
    FT_UNKNOWN,
    FT_REGULAR,  // 普通文件
    FT_DIRECTORY // 目录文件
};

/*打开文件的选项 */
enum oflags
{
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT = 4
};

/*用来记录查找文件过程中已经找到的上级路径(走过的地方) */
struct path_search_record
{
    char searched_path[MAX_PATH_LEN];
    struct dir *parent_dir;
    enum file_types file_type;
};

extern struct partition *curr_part;

static int search_file(const char *filename, struct path_search_record *searched_record);
static void partition_format(struct partition *part);
static bool mount_partition(struct list_elem *pelem, int arg);

void filesys_init(); // 初始化文件系统
static char *path_parse(char *pathname, char *name_store);
int32_t path_deepth_cnt(char *pathname);
#endif