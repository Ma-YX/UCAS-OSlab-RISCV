#include <type.h>

/**************************************************************************************************
Disk layout:
[ boot block | kernel image ] (512MB)
[ Superblock(512B) | block map(128KB) | inode map(512B) | inode blocks(256KB) | data blocks ] (512MB)
****************************************************************************************************/
//one secter||block = 512B
#define FS_MAGIC 0x190625
#define FS_SIZE  1049346 //512B + 128KB + 512B + 256KB + 512MB
#define FS_START 1048576 //512MB

#define BLOCK_MAP_OFFSET   1
#define BLOCK_MAP_NUM      256 //128KB
#define INODE_MAP_OFFSET   257
#define INODE_MAP_NUM      1   //512B
#define INODE_BLOCK_OFFSET 258
#define INODE_BLOCK_NUM    512 //256KB
#define DATA_NUM_OFFSET    770
#define DATA_BLOCK_NUM     1048576 //512MB

#define O_RD 1
#define O_WR 2
#define O_RW 3

#define T_DIR  1
#define T_FILE 2

#define MAX_DIR 10

#define MAX_NAME_LENGTH 16

typedef struct superblock{
    u32 magic;
    u32 fs_num;
    u32 fs_start_block;

    u32 block_map_offset;
    u32 block_map_num;

    u32 inode_map_offset;
    u32 inode_map_num;

    u32 inode_block_offset;
    u32 inode_block_num;

    u32 data_block_offset;
    u32 data_block_num;
}superblock_t;

typedef struct inode{
    u8  ino;
    u8  mode;
    u8  num; //link
    u16 used_size;
    u16 ctime;
    u16 mtime;
    u16 direct[MAX_DIR];
    u16 indirect1;
    u16 indirect2;
    u32 bytes; //file
}inode_t;

typedef struct dentry{
    char name[MAX_NAME_LENGTH];
    u8   type;
    i16  ino;
}dentry_t;

extern int global_inode_num;

void do_mkfs();
void do_statfs();
void do_cd(char *name);
void do_mkdir(char *name);
void do_rmdir(char *name);
void do_fs_ls(char *name);

int do_touch(char *name);
int do_cat(char *name);
int do_fopen(char *name, int access);
int do_fclose(int fd);
int do_fread(int fd, char *buff, int size);
int do_fwrite(int fd, char *buff, int size);
int do_rmfile(char *name);
int do_link(char *src, char *dst);
int do_fs_ls_l(char *name);
int do_lseek(int fd, int offset, int whence);

u32 alloc_inode();
u32 alloc_block();
int check_fs();
dentry_t find_dir(int inode_num, char *name);
dentry_t find_file(int inode_num, char *name);
u32 find_parent_dir(int inode_num, char *name);
void rm_dir(u32 inode_num);
void rm_file(u32 inode_num);
int relocate_wpos(int fd,int offset,int whence);