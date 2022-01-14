#include <os/fs.h>
#include <sbi.h>
#include <pgtable.h>
#include <os/string.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/smp.h>

int global_inode_num;

u8 empty[4096] = {0};
u8 buffer[4096] = {0};
u8 indirect1_space[4096] = {0};
u8 indirect2_space[4096] = {0};

void do_mkfs(){
    prints("[FS] Start initialize filesystem!\n");
    prints("[FS] Setting up superblock...\n");
    superblock_t sb;
    sb.magic = FS_MAGIC;
    sb.fs_num = FS_SIZE;
    sb.fs_start_block = FS_START;
    sb.block_map_offset = BLOCK_MAP_OFFSET;
    sb.block_map_num = BLOCK_MAP_NUM;
    sb.inode_map_offset = INODE_MAP_OFFSET;
    sb.inode_map_num = INODE_MAP_NUM;
    sb.inode_block_offset = INODE_BLOCK_OFFSET;
    sb.inode_block_num = INODE_BLOCK_NUM;
    sb.data_block_offset = DATA_NUM_OFFSET;
    sb.data_block_num = DATA_BLOCK_NUM;
    sbi_sd_write(kva2pa(&sb), 1, FS_START);

    prints("[FS] Checking superblock:\n");
    u8 tmp[512];
    sbi_sd_read(kva2pa(tmp), 1, FS_START);
    superblock_t *tmp_sb_p = (superblock_t *)tmp;
    prints("     magic: 0x%x\n", tmp_sb_p->magic);
    prints("     num sector: %d, start sector: %d\n", tmp_sb_p->fs_num, tmp_sb_p->fs_start_block);
    prints("     block map offset: %d, block map num: %d\n", tmp_sb_p->block_map_offset, tmp_sb_p->block_map_num);
    prints("     inode map offset: %d, inode map num: %d\n", tmp_sb_p->inode_map_offset, tmp_sb_p->inode_map_num);
    prints("     inode block offset: %d, inode block num: %d\n", tmp_sb_p->inode_block_offset, tmp_sb_p->inode_block_num);
    prints("     data block offset: %d, data block num: %d\n", tmp_sb_p->data_block_offset, tmp_sb_p->data_block_num);
    
    //initialize block map, inode map, inode , data
    //1: clear, 2: set
    kmemset(tmp, 0, 512);
    prints("[FS] Setting up block map...\n");
    for(int i = 0; i < BLOCK_MAP_NUM; i++){
        sbi_sd_write(kva2pa(tmp), 1, FS_START + BLOCK_MAP_OFFSET + i);
    }
    u32 block_num = alloc_block();

    kmemset(tmp, 0, 512);
    prints("[FS] Setting up inode map...\n");
    for(int i = 0; i < INODE_MAP_NUM; i++){
        sbi_sd_write(kva2pa(tmp), 1, FS_START + INODE_MAP_OFFSET + i);
    }
    u32 inode_num = alloc_inode();

    kmemset(tmp, 0, 512);
    prints("[FS] Setting up inode block...\n");
    for(int i = 0; i < INODE_BLOCK_NUM; i++){
        sbi_sd_write(kva2pa(tmp), 1, FS_START + INODE_BLOCK_OFFSET + i);
    }
    kmemset(tmp, 0, 512);
    prints("     inode_num = %d\n", inode_num);
    inode_t *inode_p = tmp;
    inode_p->ino = inode_num;
    inode_p->mode = O_RW;
    inode_p->num = inode_num;
    inode_p->used_size = 2;
    inode_p->ctime = get_timer();
    inode_p->mtime = get_timer();
    inode_p->direct[0] = block_num;
    sbi_sd_write(kva2pa(inode_p), 1, FS_START + INODE_BLOCK_OFFSET + inode_num);
    global_inode_num = inode_num;

    kmemset(tmp, 0, 512);
    prints("[FS] Setting up data block(root directory)...\n");
    prints("     block_num = %d\n", block_num);
    dentry_t *root_dentry = (dentry_t *)tmp;
    kmemcpy(root_dentry->name, ".", 1);
    root_dentry->type = T_DIR;
    root_dentry->ino = inode_num;
    sbi_sd_write(kva2pa(tmp), 1, FS_START + DATA_NUM_OFFSET + block_num);
    kmemset(tmp, 0, 512);
    kmemcpy(root_dentry->name, "..", 2);
    root_dentry->type = T_DIR;
    root_dentry->ino = inode_num;
    sbi_sd_write(kva2pa(tmp), 1, FS_START + DATA_NUM_OFFSET + block_num + 1);

    prints("[FS] Initialize filesystem finished.\n");
}

void do_statfs(){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return ;
    }
    u8 tmp[512];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);

    //filesystem infomation
    prints("[FS] Information:\n");
    prints("     magic: 0x%x\n", sb->magic);
    prints("     num sector: %d, start sector: %d\n", sb->fs_num, sb->fs_start_block);
    prints("     block map offset: %d, block map num: %d\n", sb->block_map_offset, sb->block_map_num);
    prints("     inode map offset: %d, inode map num: %d\n", sb->inode_map_offset, sb->inode_map_num);
    prints("     inode block offset: %d, inode block num: %d\n", sb->inode_block_offset, sb->inode_block_num);
    prints("     data block offset: %d, data block num: %d\n", sb->data_block_offset, sb->data_block_num);

    //occupied infomation
    u8 tmp1[1024];
    u8 *inode_p = tmp1;
    sbi_sd_read(kva2pa(inode_p), sb->inode_map_num, sb->fs_start_block + sb->inode_map_offset);
    int num_uesd_inode = 0;
    for(int i = 0; i < 512 * sb->inode_map_num; i++){
        if(inode_p[i]){
            num_uesd_inode++;
        }
    }
    prints("    used inode num: %d\n", num_uesd_inode);
    u8 *data_block_p = tmp1;
    int num_used_block = 0;
    for(int i = 0; i < sb->block_map_num; i++){
        sbi_sd_read(kva2pa(data_block_p), 1, sb->fs_start_block + sb->block_map_offset + i);
        for(int j = 0; j < 512; j++){
            if(data_block_p[j]){
                num_used_block++;
            }
        }
    }
    prints("    used block num: %d, used data block num: %d\n", num_used_block, num_used_block * 8);
}

void do_fs_ls(char *name){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return ;
    }
    dentry_t dy_match;
    if(name[0] == '/'){
        dy_match = find_dir(0, &name[1]);
    }else{
        dy_match = find_dir(global_inode_num, &name[1]);
    }
    if(dy_match.ino == -1){
        prints("ERROR: no such directory\n");
        return ;
    }

    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;
    inode_t *inode_p = (inode_t *)tmp;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + dy_match.ino);
    u8 dentry_buffer[512];
    for(int i = 0; i < inode_p->used_size; i++){
        dentry_t *dy = (dentry_t *)dentry_buffer;
        sbi_sd_read(kva2pa(dy), 1, data_block_start + inode_p->direct[i / 8] +  i % 8);
        if(dy->ino >= 0){
            if(dy->type == T_DIR){
                prints("[DIR]  %s\n", dy->name);
            }
            else if(dy->type == T_FILE){
                prints("[FILE] %s\n", dy->name);
            }
        }
    }
}

int do_fs_ls_l(char *name){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return -1;
    }
    dentry_t dy_match;
    if(name[0] == '/'){
        dy_match = find_dir(0, &name[1]);
    }else{
        dy_match = find_dir(global_inode_num, &name[1]);
    }
    if(dy_match.ino == -1){
        prints("ERROR: no such directory\n");
        return -1;
    }
    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;
    inode_t *inode_p = (inode_t *)tmp;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + dy_match.ino);
    u8 dentry_buffer[512];
    for(int i = 0; i < inode_p->used_size; i++){
        dentry_t *dy = (dentry_t *)dentry_buffer;
        sbi_sd_read(kva2pa(dy), 1, data_block_start + inode_p->direct[i / 8] +  i % 8);
        if(dy->ino >= 0){
            inode_t *tar_inode = (inode_t *)buffer;
            sbi_sd_read(kva2pa(tar_inode), 1, inode_block_start + dy->ino);
            if(dy->type == T_DIR){
                prints("[DIR]  %s\n", dy->name);
                prints("      occupied space: %d Blocks / %d Bytes\n",tar_inode->used_size, tar_inode->used_size * 512);
                prints("      written  space: %d Blocks / %d Bytes\n",tar_inode->bytes / 512 + 1, tar_inode->bytes);
            }
            else if(dy->type == T_FILE){
                prints("[FILE] %s\n", dy->name);
                prints("      occupied space: %d Blocks / %d Bytes\n",tar_inode->used_size, tar_inode->used_size * 512);
                prints("      written  space: %d Blocks / %d Bytes\n",tar_inode->bytes / 512 + 1, tar_inode->bytes);
                prints("      link:           %d\n",tar_inode->num);
            }
        }
    }
}

void do_cd(char *name){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return ;
    }
    dentry_t dy_match;
    if(name[0] == '/'){
        dy_match = find_dir(0, &name[1]);
    }else{
        dy_match = find_dir(global_inode_num, &name[0]);
    }
    if(dy_match.ino == -1){
        prints("ERROR: no such directory\n");
        return ;
    }else{
        global_inode_num = dy_match.ino;
    }
}

void do_mkdir(char *name){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return ;
    }
    if (!kstrcmp(name, ".") || !kstrcmp(name, "..")){
        prints("ERROR: illegal name\n");
        return ;
    }

    dentry_t dy_match = find_dir(global_inode_num, name);
    if(dy_match.ino >= 0){
        prints("ERROR: directory has exited\n");
    }

    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;

    u8 inode_buffer[512];
    inode_t *inode_p = (inode_t *)inode_buffer;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + global_inode_num);
    //check space
    if(inode_p->used_size == MAX_DIR * 8){
        prints("ERROR: no space left for new directory\n");
        return ;
    }
    inode_p->used_size++;
    inode_p->num++;
    //set new dentry
    if(inode_p->used_size % 8 == 0){
        inode_p->direct[inode_p->used_size / 8] = alloc_block();
    }
    inode_p->mtime = get_timer();
    sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + global_inode_num);

    u8 dentry_buffer[512];
    kmemset(dentry_buffer, 0, 512);
    dentry_t *dy = (dentry_t *)dentry_buffer;
    dy->type = T_DIR;
    dy->ino = alloc_inode();
    kmemcpy(dy->name, name, kstrlen(name));
    sbi_sd_write(kva2pa(dy), 1, data_block_start + inode_p->direct[(inode_p->used_size - 1) / 8] + (inode_p->used_size - 1) % 8);

    int new_ino = dy->ino;
    kmemset(inode_buffer, 0, 512);
    inode_t *new_inode = (inode_t *)inode_buffer;
    new_inode->ino = new_ino;
    new_inode->mode = O_RW;
    new_inode->num = 0;
    new_inode->used_size = 2;
    new_inode->ctime = get_timer();
    new_inode->mtime = get_timer();
    new_inode->direct[0] = alloc_block();
    sbi_sd_write(kva2pa(new_inode), 1, inode_block_start + new_ino);

    kmemset(dentry_buffer, 0, 512);
    dy = (dentry_t *)dentry_buffer;
    kmemcpy(dy->name, ".", 1);
    dy->type = T_DIR;
    dy->ino  =new_ino;
    sbi_sd_write(kva2pa(dy), 1, data_block_start + new_inode->direct[0]);
    kmemcpy(dy->name, "..", 2);
    dy->type = T_DIR;
    dy->ino  =global_inode_num;
    sbi_sd_write(kva2pa(dy), 1, data_block_start + new_inode->direct[0] + 1);
}

void do_rmdir(char *name){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return ;
    }
    if (!kstrcmp(name, ".") || !kstrcmp(name, "..")){
        prints("ERROR: illegal name\n");
        return ;
    }

    dentry_t dy_match;
    if(name[0] == '/'){
        dy_match = find_dir(0, &name[1]);
    }else{
        dy_match = find_dir(global_inode_num, &name[0]);
    }
    if(dy_match.ino == -1){
        prints("ERROR: no such directory\n");
        return ;
    }

    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_map_start = sb->inode_map_offset + fs_block_start;
    u32 block_map_start = sb->block_map_offset + fs_block_start;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;

    inode_t *inode_p = (inode_t *)tmp;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + dy_match.ino);

    //locate parent
    u8 dentry_buffer[512];
    u32 parent_inode;
    for(int i = 0; i < inode_p->used_size; i++){
        dentry_t *dy = (dentry_t *)dentry_buffer;
        sbi_sd_read(kva2pa(dy), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
        if(kstrcmp(dy->name, "..") == 0){
            parent_inode = dy->ino;
        }
    }

    //rm dir
    rm_dir(dy_match.ino);

    //modify parent
    u8 inode_buffer[512];
    inode_t *parent = (inode_t *)inode_buffer;
    sbi_sd_read(kva2pa(parent), 1, inode_block_start + parent_inode);
    kmemset(dentry_buffer, 0, 512);
    for(int i = 0; i < parent->used_size; i++){
        dentry_t *dy = (dentry_t *)dentry_buffer;
        sbi_sd_read(kva2pa(dy), 1, data_block_start + parent->direct[i / 8] + i % 8);
        if((kstrcmp(dy->name, dy_match.name) == 0) && dy->type == T_DIR){
            sbi_sd_write(kva2pa(empty), 1, data_block_start + parent->direct[i / 8] + i % 8);
            //remap
            sbi_sd_read(kva2pa(buffer), 1, data_block_start + parent->direct[(parent->used_size - 1) / 8] + (parent->used_size - 1) % 8);
            sbi_sd_write(kva2pa(buffer), 1, data_block_start + parent->direct[i / 8] + i % 8);
            if((parent->used_size - 1) % 8 == 0){
                u32 block_map_limit = block_map_start + (parent->direct[(parent->used_size - 1) / 8] / 8) / 512;
                sbi_sd_read(kva2pa(buffer), 1, block_map_limit);
                buffer[(parent->direct[(parent->used_size - 1) / 8] / 8) % 512] = 0;
                sbi_sd_write(kva2pa(buffer), 1, block_map_limit);
            }
            parent->used_size--;
            parent->num--;
            parent->mtime = get_timer();
            sbi_sd_write(kva2pa(parent), 1, inode_block_start + parent_inode);
            break;
        }
    }
}

int do_rmfile(char *name){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return -1;
    }
    dentry_t dy_match;
    if(name[0] == '/'){
        dy_match = find_file(0, &name[1]);
    }else{
        dy_match = find_file(global_inode_num, &name[0]);
    }
    if(dy_match.ino == -1){
        prints("ERROR: no such file\n");
        return -1;
    }

    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_map_start = sb->inode_map_offset + fs_block_start;
    u32 block_map_start = sb->block_map_offset + fs_block_start;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;

    inode_t *inode_p = (inode_t *)tmp;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + dy_match.ino);
    //locate parent
    u8 dentry_buffer[512];
    u32 parent_inode;
    if(name[0] == '/'){
        parent_inode = find_parent_dir(0, &name[1]);
    }else{
        parent_inode = find_parent_dir(global_inode_num, &name[0]);
    }

    //rm file
    rm_file(dy_match.ino);

    //modify parent
    u8 inode_buffer[512];
    inode_t *parent = (inode_t *)inode_buffer;
    sbi_sd_read(kva2pa(parent), 1, inode_block_start + parent_inode);
    kmemset(dentry_buffer, 0, 512);
    for(int i = 0; i < parent->used_size; i++){
        dentry_t *dy = (dentry_t *)dentry_buffer;
        sbi_sd_read(kva2pa(dy), 1, data_block_start + parent->direct[i / 8] + i % 8);
        if((kstrcmp(dy->name, dy_match.name) == 0) && dy->type == T_FILE){
            sbi_sd_write(kva2pa(empty), 1, data_block_start + parent->direct[i / 8] + i % 8);
            //remap
            sbi_sd_read(kva2pa(buffer), 1, data_block_start + parent->direct[(parent->used_size - 1) / 8] + (parent->used_size - 1) % 8);
            sbi_sd_write(kva2pa(buffer), 1, data_block_start + parent->direct[i / 8] + i % 8);
            if((parent->used_size - 1) % 8 == 0){
                u32 block_map_limit = block_map_start + (parent->direct[(parent->used_size - 1) / 8] / 8) / 512;
                sbi_sd_read(kva2pa(buffer), 1, block_map_limit);
                buffer[(parent->direct[(parent->used_size - 1) / 8] / 8) % 512] = 0;
                sbi_sd_write(kva2pa(buffer), 1, block_map_limit);
            }
            parent->used_size--;
            parent->num--;
            parent->mtime = get_timer();
            sbi_sd_write(kva2pa(parent), 1, inode_block_start + parent_inode);
            break;
        }
    }
}

void rm_dir(u32 inode_num){
    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_map_start = sb->inode_map_offset + fs_block_start;
    u32 block_map_start = sb->block_map_offset + fs_block_start;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;

    inode_t *inode_p = (inode_t *)tmp;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + inode_num);
    
    u8 dentry_buffer[512];
    for(int i = 0; i < inode_p->used_size; i++){
        dentry_t *dy = (dentry_t *)dentry_buffer;
        sbi_sd_read(kva2pa(dy), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
        if(kstrcmp(dy->name, ".") == 0 || kstrcmp(dy->name, "..") == 0){
            sbi_sd_write(kva2pa(empty), 1 , data_block_start + inode_p->direct[i / 8] + i % 8);
        }else if(dy->type == T_DIR){
            rm_dir(dy->ino);
            sbi_sd_write(kva2pa(empty), 1 , data_block_start + inode_p->direct[i / 8] + i % 8);
        }else if(dy->type == T_FILE){
            rm_file(dy->ino);
            sbi_sd_write(kva2pa(empty), 1 , data_block_start + inode_p->direct[i / 8] + i % 8);
        }
        //clear block map
        if(i % 8 == 0){
            u32 block_map_limit = block_map_start + (inode_p->direct[(inode_p->used_size - 1) / 8] / 8) / 512;
            sbi_sd_read(kva2pa(buffer), 1, block_map_limit);   
            buffer[(inode_p->direct[(inode_p->used_size - 1) / 8] / 8) % 512] = 0;
            sbi_sd_write(kva2pa(buffer), 1, block_map_limit);
        }
    }
    //clear inode map
    sbi_sd_write(kva2pa(empty), 1, inode_block_start + inode_p->ino);
    sbi_sd_read(kva2pa(buffer), 1, inode_map_start);
    buffer[inode_p->ino] = 0;
    sbi_sd_write(kva2pa(buffer), 1, inode_map_start);
}

void rm_file(u32 inode_num){
    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_map_start = sb->inode_map_offset + fs_block_start;
    u32 block_map_start = sb->block_map_offset + fs_block_start;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;

    inode_t *inode_p = (inode_t *)tmp;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + inode_num);

    if(inode_p->num > 0){
        inode_p->num--;
        sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + inode_num);
        return ;
    }

    for(int i = 0; i < MAX_DIR * 8; i++){
        if(inode_p->direct[i / 8] != 0){
            sbi_sd_write(kva2pa(empty), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
        }
        if(i % 8 == 0){
            u32 block_map_limit = block_map_start + (inode_p->direct[i / 8] / 8) / 512;
            sbi_sd_read(kva2pa(buffer), 1, block_map_limit);
            buffer[(inode_p->direct[i / 8] / 8) % 512] = 0;
            sbi_sd_write(kva2pa(buffer), 1, block_map_limit);
        }
    }

    if(inode_p->indirect1 != 0){
        u32 *indirect1 = (u32 *)indirect1_space;
        sbi_sd_read(kva2pa(indirect1), 8, data_block_start + inode_p->indirect1);
        for(int i = 0; i < 1024; i++){
            if(indirect1_space[i] != 0){
                sbi_sd_write(kva2pa(empty), 8, data_block_start + indirect1_space[i]);
                u32 block_map_limit = block_map_start + (indirect1_space[i] / 8) / 512;
                sbi_sd_read(kva2pa(buffer), 1, block_map_limit);
                buffer[(indirect1_space[i] / 8) % 512] = 0;
                sbi_sd_write(kva2pa(buffer), 1, block_map_limit);
            }
        }
        u32 block_map_limit = block_map_start + (inode_p->indirect1 / 8) / 512;
        sbi_sd_read(kva2pa(buffer), 1, block_map_limit);
        buffer[(inode_p->indirect1 / 8) % 512] = 0;
        sbi_sd_write(kva2pa(buffer), 1, block_map_limit);
    }

    if(inode_p->indirect2 != 0){
        u32 *indirect2 = (u32 *)indirect2_space;
        sbi_sd_read(kva2pa(indirect2), 8, data_block_start + inode_p->indirect2);
        for(int j = 0; j < 1024; j++){
            if(indirect2_space[j] != 0){
                u32 *indirect1 = (u32 *)indirect1_space;
                sbi_sd_read(kva2pa(indirect1), 8, data_block_start + indirect2_space[j]);
                for(int i = 0; i < 1024; i++){
                    if(indirect1_space[i] != 0){
                        sbi_sd_write(kva2pa(empty), 8, data_block_start + indirect1_space[i]);
                        u32 block_map_limit = block_map_start + (indirect1_space[i] / 8) / 512;
                        sbi_sd_read(kva2pa(buffer), 1, block_map_limit);
                        buffer[(indirect1_space[i] / 8) % 512] = 0;
                        sbi_sd_write(kva2pa(buffer), 1, block_map_limit);
                    }
                }
                u32 block_map_limit = block_map_start + (indirect2_space[j] / 8) / 512;
                sbi_sd_read(kva2pa(buffer), 1, block_map_limit);
                buffer[(indirect2_space[j] / 8) % 512] = 0;
                sbi_sd_write(kva2pa(buffer), 1, block_map_limit);
            }
        }
        u32 block_map_limit = block_map_start + (inode_p->indirect2 / 8) / 512;
        sbi_sd_read(kva2pa(buffer), 1, block_map_limit);
        buffer[(inode_p->indirect2 / 8) % 512] = 0;
        sbi_sd_write(kva2pa(buffer), 1, block_map_limit);
    }

    sbi_sd_write(kva2pa(empty), 1, inode_block_start + inode_p->ino);
    sbi_sd_read(kva2pa(buffer), 1, inode_map_start);
    buffer[inode_p->ino] = 0;
    sbi_sd_write(kva2pa(buffer), 1, inode_map_start);
}

int do_touch(char *name){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return ;
    }
    if (!kstrcmp(name, ".") || !kstrcmp(name, "..")){
        prints("ERROR: illegal name\n");
        return ;
    }

    dentry_t dy_match = find_file(global_inode_num, name);
    if(dy_match.ino >= 0){
        prints("ERROR: file has exited\n");
    }

    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;

    u8 inode_buffer[512];
    inode_t *inode_p = (inode_t *)inode_buffer;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + global_inode_num);

    //alloc inode block
    if(inode_p->used_size == 8 * MAX_DIR){
        prints("ERROR: no space left for new file\n");
    }
    inode_p->used_size++;
    inode_p->num++;
    if(inode_p->used_size % 8 == 0){
        inode_p->direct[inode_p->used_size / 8] = alloc_block();
    }
    inode_p->mtime = get_timer();
    sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + global_inode_num);
    //dentry
    u8 dentry_buffer[512];
    kmemset(dentry_buffer, 0, 512);
    dentry_t *dy = (dentry_t *)dentry_buffer;
    dy->type = T_FILE;
    dy->ino = alloc_inode();
    kmemcpy(dy->name, name, kstrlen(name));
    sbi_sd_write(kva2pa(dy), 1, data_block_start + inode_p->direct[(inode_p->used_size - 1) / 8] + (inode_p->used_size - 1) % 8);
    
    u32 new_ino = dy->ino;
    kmemset(inode_buffer, 0, 512);
    inode_t *new_inode = (inode_t *)inode_buffer;
    new_inode->ino = new_ino;
    new_inode->mode = O_RW;
    new_inode->num= 0;
    new_inode->used_size = 1;
    new_inode->ctime = get_timer();
    new_inode->mtime = get_timer();
    new_inode->direct[0] = alloc_block();
    new_inode->bytes = 0;
    sbi_sd_write(kva2pa(new_inode), 1, inode_block_start + new_ino);

    sbi_sd_read(kva2pa(buffer), 1, data_block_start + new_inode->direct[0]);
    kmemcpy(buffer, "new file", 8);
    sbi_sd_write(kva2pa(buffer), 1, data_block_start + new_inode->direct[0]);
}

int do_cat(char *name){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return ;
    }
    dentry_t dy_match;
    if(name[0] == '/'){
        dy_match = find_file(0, &name[1]);
    }else{
        dy_match = find_file(global_inode_num, &name[0]);
    }
    if(dy_match.ino == -1){
        prints("ERROR: no such file\n");
        return ;
    }
    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;
    inode_t *inode_p = (inode_t *)tmp;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + dy_match.ino);
    for(int i = 0; i < inode_p->used_size; i++){
        sbi_sd_read(kva2pa(buffer), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
        prints("%s", buffer);
    }
}

int check_fs(){
    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    if(sb->magic == FS_MAGIC){
        return 1;
    }else{
        return 0;
    }
}

u32 alloc_block(){
    u32 alloc_block_num;
    u8 tmp[1024];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 block_map_start = sb->fs_start_block + sb->block_map_offset;
    u32 block_map_num = sb->block_map_num;
    for(int i = 0; i < block_map_num; i++){
        sbi_sd_read(kva2pa(tmp), 1, block_map_start + i);
        for(int j = 0; j < 512; j++){
            if(tmp[j] == 0){
                tmp[j] = 1;
                alloc_block_num = (u32)((i * 512 + j) * 8);
                sbi_sd_write(kva2pa(tmp), 1, block_map_start + i);
                return alloc_block_num;
            }
        }
    }
    prints("ERROR: too few blocks\n");
}

u32 alloc_inode(){
    u32 alloc_inode_num;
    u8 tmp[512];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 inode_map_start = sb->fs_start_block + sb->inode_map_offset;
    u32 inode_map_num = sb->inode_map_num;
    sbi_sd_read(kva2pa(tmp), inode_map_num, inode_map_start);
    for(int i = 0; i < 512 * inode_map_num; i++){
        if(tmp[i] == 0){
            tmp[i] = 1;
            alloc_inode_num = (u32)i;
            break;
        }
    }
    sbi_sd_write(kva2pa(tmp), inode_map_num, inode_map_start);
    return alloc_inode_num;
}

dentry_t find_dir(int inode_num, char *name){
    dentry_t dir_match;
    int length;
    length = kstrlen(name);
    if(length == 0){
        dir_match.ino = inode_num;
        return dir_match;
    }
    int dir_1 = 0;
    int dir_num = 0;
    for(int i = 0; i < length; i++){
        if(name[i] == '/'){
            if(dir_1 == 0){
                dir_1 = i;
            }
            dir_num++;
        }
    }
    char first_name [MAX_NAME_LENGTH] = {0};
    char left_name [MAX_NAME_LENGTH] = {0};
    kmemcpy(first_name, name, dir_1);
    kmemcpy(left_name, &name[dir_1 + 1], kstrlen(name) - dir_1 - 1);

    u8 tmp[512];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 inode_block_start = sb->fs_start_block + sb->inode_block_offset;
    u32 data_block_start = sb->fs_start_block + sb->data_block_offset;

    inode_t *inode_p = (inode_t *)tmp;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + inode_num);

    if(dir_num == 0){
        u8 dentry_buffer[512];
        for(int i = 0; i < inode_p->used_size; i++){
            dentry_t *dy = (dentry_t *)dentry_buffer;
            sbi_sd_read(kva2pa(dy), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
            if(!kstrcmp(dy->name, name) && dy->ino >= 0 && dy->type == T_DIR){
                kmemcpy(dir_match.name, name, kstrlen(name));
                dir_match.ino = dy->ino;
                dir_match.type = dy->type;
                return dir_match;
            }
        }
    }else{
        u8 dentry_buffer[512];
        for(int i = 0; i < inode_p->used_size; i++){
            dentry_t *dy = (dentry_t *)dentry_buffer;
            sbi_sd_read(kva2pa(dy), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
            if(!kstrcmp(dy->name, first_name) && dy->ino >= 0 && dy->type == T_DIR){
                return find_dir(dy->ino, left_name);
            }
        }
    }
    dir_match.ino = -1;
    return dir_match;
}

dentry_t find_file(int inode_num, char *name){
    dentry_t dir_match;
    int length;
    length = kstrlen(name);
    if(length == 0){
        dir_match.ino = inode_num;
        return dir_match;
    }
    int dir_1 = 0;
    int dir_num = 0;
    for(int i = 0; i < length; i++){
        if(name[i] == '/'){
            if(dir_1 == 0){
                dir_1 = i;
            }
            dir_num++;
        }
    }
    char first_name [MAX_NAME_LENGTH] = {0};
    char left_name [MAX_NAME_LENGTH] = {0};
    kmemcpy(first_name, name, dir_1);
    kmemcpy(left_name, &name[dir_1 + 1], kstrlen(name) - dir_1 - 1);

    u8 tmp[512];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 inode_block_start = sb->fs_start_block + sb->inode_block_offset;
    u32 data_block_start = sb->fs_start_block + sb->data_block_offset;

    inode_t *inode_p = (inode_t *)tmp;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + inode_num);

    if(dir_num == 0){
        u8 dentry_buffer[512];
        for(int i = 0; i < inode_p->used_size; i++){
            dentry_t *dy = (dentry_t *)dentry_buffer;
            sbi_sd_read(kva2pa(dy), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
            if(!kstrcmp(dy->name, name) && dy->ino >= 0 && dy->type == T_FILE){
                kmemcpy(dir_match.name, name, kstrlen(name));
                dir_match.ino = dy->ino;
                dir_match.type = dy->type;
                return dir_match;
            }
        }
    }else{
        u8 dentry_buffer[512];
        for(int i = 0; i < inode_p->used_size; i++){
            dentry_t *dy = (dentry_t *)dentry_buffer;
            sbi_sd_read(kva2pa(dy), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
            if(!kstrcmp(dy->name, first_name) && dy->ino >= 0 && dy->type == T_DIR){
                return find_file(dy->ino, left_name);
            }
        }
    }
    dir_match.ino = -1;
    return dir_match;
}

u32 find_parent_dir(int inode_num, char *name){
    dentry_t dir_match;
    int length;
    length = kstrlen(name);
    if(length == 0){
        dir_match.ino = inode_num;
        return dir_match.ino;
    }
    int dir_1 = 0;
    int dir_num = 0;
    for(int i = 0; i < length; i++){
        if(name[i] == '/'){
            if(dir_1 == 0){
                dir_1 = i;
            }
            dir_num++;
        }
    }
    char first_name [MAX_NAME_LENGTH] = {0};
    char left_name [MAX_NAME_LENGTH] = {0};
    kmemcpy(first_name, name, dir_1);
    kmemcpy(left_name, &name[dir_1 + 1], kstrlen(name) - dir_1 - 1);

    u8 tmp[512];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 inode_block_start = sb->fs_start_block + sb->inode_block_offset;
    u32 data_block_start = sb->fs_start_block + sb->data_block_offset;

    inode_t *inode_p = (inode_t *)tmp;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + inode_num);

    if(dir_num == 0){
        u8 dentry_buffer[512];
        for(int i = 0; i < inode_p->used_size; i++){
            dentry_t *dy = (dentry_t *)dentry_buffer;
            sbi_sd_read(kva2pa(dy), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
            if(!kstrcmp(dy->name, name) && dy->ino >= 0 && dy->type == T_FILE){
                kmemcpy(dir_match.name, name, kstrlen(name));
                dir_match.ino = dy->ino;
                dir_match.type = dy->type;
                return inode_p->ino;
            }
        }
    }else{
        u8 dentry_buffer[512];
        for(int i = 0; i < inode_p->used_size; i++){
            dentry_t *dy = (dentry_t *)dentry_buffer;
            sbi_sd_read(kva2pa(dy), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
            if(!kstrcmp(dy->name, first_name) && dy->ino >= 0 && dy->type == T_DIR){
                return find_parent_dir(dy->ino, left_name);
            }
        }
    }
    dir_match.ino = -1;
    return dir_match.ino;
}

int do_fopen(char *name, int access){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return -1;
    }
    dentry_t dy_match;
    if(name[0] == '/'){
        dy_match = find_file(0, &name[1]);
    }else{
        dy_match = find_file(global_inode_num, &name[0]);
    }
    if(dy_match.ino == -1){
        prints("ERROR: no such file\n");
        return -1;
    }
    //set file in pcb
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    int tag;
    for(int i = 0; i < NUM_MAX_FILE; i++){
        tag = i;
        if(current_running->file_list[i].valid == 0){
            break;
        }
    }
    if(tag == NUM_MAX_FILE){
        prints("ERROR: no space left for new file in pcb\n");
    }else{
        current_running->file_list[tag].inode = dy_match.ino;
        current_running->file_list[tag].access = access;
        //current_running->file_list[tag].addr = 0;
        current_running->file_list[tag].rd_pos = 0;
        current_running->file_list[tag].wr_pos = 0;
        current_running->file_list[tag].valid = 1;
        return tag;
    }
}

int do_fclose(int fd){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    if(fd >= NUM_MAX_FILE || fd < 0){
        prints("ERROR: illegal fd\n");
        return -1;
    }else{
        current_running->file_list[fd].valid = 0;
        return 1;
    }
}

int do_fread(int fd, char *buff, int size){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return -1;
    }
    if(fd >= NUM_MAX_FILE || fd < 0){
        prints("ERROR: illegal fd\n");
        return -1;
    }
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    if(current_running->file_list[fd].valid == 0){
        prints("ERROR: open file first\n");
    }

    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_map_start = sb->inode_map_offset + fs_block_start;
    u32 block_map_start = sb->block_map_offset + fs_block_start;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;

    inode_t *inode_p = (inode_t *)tmp;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
    int left_size = size;
    //start read 
    //enough bytes in current sector
    if(current_running->file_list[fd].rd_pos % 512 != 0){
        //direct map 40KB
        if(current_running->file_list[fd].rd_pos < MAX_DIR * 4096){
            int judge = left_size - (512 - current_running->file_list[fd].rd_pos % 512);
            int valid_bytes = (judge < 0) ? left_size : (512 - current_running->file_list[fd].rd_pos % 512);
            sbi_sd_read(kva2pa(buffer), 1, data_block_start + inode_p->direct[(current_running->file_list[fd].rd_pos / 512) / 8] + (current_running->file_list[fd].rd_pos / 512) % 8);
            kmemcpy(buff, &buffer[current_running->file_list[fd].rd_pos % 512], valid_bytes);
            left_size -= valid_bytes;
            buff += valid_bytes;
            current_running->file_list[fd].rd_pos += valid_bytes;
        }
        //first indirect map 40MB
        else if(current_running->file_list[fd].rd_pos < MAX_DIR * 4096 + 1024 * 4096){
            u32 *indirect1_p = (u32 *)buffer;
            sbi_sd_read(kva2pa(indirect1_p), 8, data_block_start + inode_p->indirect1);
            int pos_1 = current_running->file_list[fd].rd_pos - MAX_DIR * 4096;
            int judge = left_size - (512 - current_running->file_list[fd].rd_pos % 512);
            int valid_bytes = (judge < 0) ? left_size : (512 - current_running->file_list[fd].rd_pos % 512);
            sbi_sd_read(kva2pa(indirect1_space), 1, data_block_start + indirect1_p[pos_1 / 512 / 8] + (pos_1 / 512) % 8);
            kmemcpy(buff, &indirect1_space[current_running->file_list[fd].rd_pos % 512], valid_bytes);
            left_size -= valid_bytes;
            buff += valid_bytes;
            current_running->file_list[fd].rd_pos += valid_bytes;
        }
        //second indirect map
        else{
            u32 *indirect2_p = (u32 *)buffer;
            u32 *indirect1_p = (u32 *)indirect1_space;
            sbi_sd_read(kva2pa(indirect2_p), 8, data_block_start + inode_p->indirect2);
            int pos_2 = current_running->file_list[fd].rd_pos -  MAX_DIR * 4096 - 1024 * 4096;
            sbi_sd_read(kva2pa(indirect1_p), 8, data_block_start + indirect2_p[pos_2 / (1024 * 4096)]);
            int pos_1 = pos_2 % (1024 * 4096);
            int judge = left_size - (512 - current_running->file_list[fd].rd_pos % 512);
            int valid_bytes = (judge < 0) ? left_size : (512 - current_running->file_list[fd].rd_pos % 512);
            sbi_sd_read(kva2pa(indirect2_space), 1, data_block_start + indirect1_p[pos_1 / 512 / 8] + (pos_1 / 512) % 8);
            kmemcpy(buff, &indirect2_space[current_running->file_list[fd].rd_pos % 512], valid_bytes);
            left_size -= valid_bytes;
            buff += valid_bytes;
            current_running->file_list[fd].rd_pos += valid_bytes;
        }
    }
    //need extra sector
    if(left_size == 0){
        return size;
    }
    //direct map
    else{
         for(int i = current_running->file_list[fd].rd_pos / 512; (i < MAX_DIR * 8) && left_size > 0; i++){
            if(inode_p->direct[i / 8] == 0){
               prints("ERROR: uninitialized block\n");
               return left_size - size;
            }
            sbi_sd_read(kva2pa(buffer), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
            int sz = (left_size > 512) ? 512 : left_size;
            kmemcpy(buff, buffer, sz);
            current_running->file_list[fd].rd_pos += sz;
            buff += sz;
            left_size -= sz;
         }
    }
    //indirect map
    if(left_size <= 0){
        return size;
    }
    else{
        if(inode_p->indirect1 == 0){
            prints("ERROR: uninitialized block\n");
            return left_size - size;
        }
        u32 *indirect1_p = (u32 *)indirect1_space;
        sbi_sd_read(kva2pa(indirect1_p), 8, data_block_start + inode_p->indirect1);
        for(int i = current_running->file_list[fd].rd_pos - MAX_DIR * 4096; (i < 1024 * 4096) && (left_size > 0); i += 512){
            if(indirect1_p[i / 512 / 8] == 0){
                prints("ERROR: uninitialized block\n");
                return left_size - size;
            }
            sbi_sd_read(kva2pa(buffer), 1, data_block_start + indirect1_p[i / 512 / 8] + (i / 512) % 8);
            int sz = (left_size > 512) ? 512 : left_size;
            kmemcpy(buff, buffer, sz);
            current_running->file_list[fd].rd_pos += sz;
            buff += sz;
            left_size -= sz;
        }
    }
    if(left_size <= 0){
        return size;
    }
    else{
        if(inode_p->indirect2 == 0){
            prints("ERROR: uninitialized block\n");
            return left_size - size;
        }
        u32 *indirect2_p = (u32 *)indirect2_space;
        sbi_sd_read(kva2pa(indirect2_p), 8, data_block_start + inode_p->indirect2);
        for(int j = (current_running->file_list[fd].rd_pos - MAX_DIR * 4096 - 1024 * 4096) / (1024 * 4096); j < 1024; j++){
            if(indirect2_p[j] == 0){
                prints("ERROR: uninitialized block\n");
                return left_size - size;
            }
            u32 *indirect1_p = (u32 *)indirect1_space;
            sbi_sd_read(kva2pa(indirect1_p), 8, data_block_start + indirect2_p[j]);
            for(int i = (current_running->file_list[fd].rd_pos - MAX_DIR * 4096 - 1024 * 4096) % (1024 * 4096); (i < 1024 * 4096) && (left_size > 0); i += 512){
                if(indirect1_p[i / 512 / 8] == 0){
                    prints("ERROR: uninitialized block\n");
                    return left_size - size;
                }
                sbi_sd_read(kva2pa(buffer), 1, data_block_start + indirect1_p[i / 512 / 8] + (i / 512) % 8);
                int sz = (left_size > 512) ? 512 : left_size;
                kmemcpy(buff, buffer, sz);
                current_running->file_list[fd].rd_pos += sz;
                buff += sz;
                left_size -= sz;
            }
        }
    }
}

int do_fwrite(int fd, char *buff, int size){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return -1;
    }
    if(fd >= NUM_MAX_FILE || fd < 0){
        prints("ERROR: illegal fd\n");
        return -1;
    }
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    if(current_running->file_list[fd].valid == 0){
        prints("ERROR: open file first\n");
    }

    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_map_start = sb->inode_map_offset + fs_block_start;
    u32 block_map_start = sb->block_map_offset + fs_block_start;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;
    int left_size = size;
    inode_t *inode_p = (inode_t *)tmp;
LOOP:
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);

    if(current_running->file_list[fd].wr_pos % 512 != 0){
        //direct
        if(current_running->file_list[fd].wr_pos < MAX_DIR * 4096){
            if(inode_p->direct[current_running->file_list[fd].wr_pos / 512 / 8] == 0){
                int pos = current_running->file_list[fd].wr_pos;
                relocate_wpos(fd, current_running->file_list[fd].wr_pos - current_running->file_list[fd].wr_pos % 512, 0);
                do_fwrite(fd, empty, 1);
                current_running->file_list[fd].wr_pos = pos;
                goto LOOP;
            }
            sbi_sd_read(kva2pa(buffer), 1, data_block_start + inode_p->direct[current_running->file_list[fd].wr_pos / 512 / 8] + (current_running->file_list[fd].wr_pos / 512) % 8);
            int judge = left_size - (512 - current_running->file_list[fd].wr_pos % 512);
            int valid_bytes = (judge < 0) ? left_size : (512 - current_running->file_list[fd].wr_pos % 512);
            kmemcpy(&buffer[current_running->file_list[fd].wr_pos % 512], buff, valid_bytes);
            sbi_sd_write(kva2pa(buffer), 1, data_block_start + inode_p->direct[current_running->file_list[fd].wr_pos / 512 / 8] + (current_running->file_list[fd].wr_pos / 512) % 8);
            left_size -= valid_bytes;
            buff += valid_bytes;
            current_running->file_list[fd].wr_pos += valid_bytes;
        }
        if(current_running->file_list[fd].wr_pos > inode_p->bytes){
            inode_p->bytes = current_running->file_list[fd].wr_pos;
            sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
            sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
        }
        //indirect1
        else if(current_running->file_list[fd].wr_pos < MAX_DIR * 4096 + 1024 * 4096){
            if(inode_p->indirect1 == 0){
                int pos = current_running->file_list[fd].wr_pos;
                relocate_wpos(fd, current_running->file_list[fd].wr_pos - current_running->file_list[fd].wr_pos % 512, 0);
                do_fwrite(fd, empty, 1);
                current_running->file_list[fd].wr_pos = pos;
                goto LOOP;
            }
            u32 *indirect1_p = (u32 *)indirect1_space;
            sbi_sd_read(kva2pa(indirect1_p), 8, data_block_start + inode_p->indirect1);
            int pos_1 = current_running->file_list[fd].wr_pos - MAX_DIR * 4096;
            if(indirect1_p[pos_1 / 512 / 8] == 0){
                int pos = current_running->file_list[fd].wr_pos;
                relocate_wpos(fd, current_running->file_list[fd].wr_pos - current_running->file_list[fd].wr_pos % 512, 0);
                do_fwrite(fd, empty, 1);
                current_running->file_list[fd].wr_pos = pos;
                goto LOOP;
            }
            sbi_sd_read(kva2pa(buffer), 1, data_block_start + indirect1_p[pos_1 / 512 / 8] + (pos_1 / 512) % 8);
            int judge = left_size - (512 - current_running->file_list[fd].wr_pos % 512);
            int valid_bytes = (judge < 0) ? left_size : (512 - current_running->file_list[fd].wr_pos % 512);
            kmemcpy(&buffer[current_running->file_list[fd].wr_pos % 512], buff, valid_bytes);
            sbi_sd_write(kva2pa(buffer), 1, data_block_start + indirect1_p[pos_1 / 512 / 8] + (pos_1 / 512) % 8);
            left_size -= valid_bytes;
            buff += valid_bytes;
            current_running->file_list[fd].wr_pos += valid_bytes;
            if(current_running->file_list[fd].wr_pos > inode_p->bytes){
                inode_p->bytes = current_running->file_list[fd].wr_pos;
                sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
                sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
            }
        }
        //indirect2
        else{
            if(inode_p->indirect2 == 0){
                int pos = current_running->file_list[fd].wr_pos;
                relocate_wpos(fd, current_running->file_list[fd].wr_pos - current_running->file_list[fd].wr_pos % 512, 0);
                do_fwrite(fd, empty, 1);
                current_running->file_list[fd].wr_pos = pos;
                goto LOOP;
            }
            u32 *indirect2_p = (u32 *)indirect2_space;
            sbi_sd_read(kva2pa(indirect2_p), 8, data_block_start + inode_p->indirect2);
            int pos_2 = current_running->file_list[fd].wr_pos - MAX_DIR * 4096 - 1024 * 4096;
            if(indirect2_p[pos_2 / (1024 * 4096)] == 0){
                int pos = current_running->file_list[fd].wr_pos;
                relocate_wpos(fd, current_running->file_list[fd].wr_pos - current_running->file_list[fd].wr_pos % 512, 0);
                do_fwrite(fd, empty, 1);
                current_running->file_list[fd].wr_pos = pos;
                goto LOOP;
            }
            u32 *indirect1_p = (u32 *)indirect2_space;
            sbi_sd_read(kva2pa(indirect1_p), 8, data_block_start + indirect2_p[pos_2 / (1024 * 4096)]);
            int pos_1 = pos_2 % (1024 * 4096);
            if(indirect1_p[pos_1 / 512 / 8] == 0){
                int pos = current_running->file_list[fd].wr_pos;
                relocate_wpos(fd, current_running->file_list[fd].wr_pos - current_running->file_list[fd].wr_pos % 512, 0);
                do_fwrite(fd, empty, 1);
                current_running->file_list[fd].wr_pos = pos;
                goto LOOP;
            }
            sbi_sd_read(kva2pa(buffer), 1, data_block_start + indirect1_p[pos_1 / 512 / 8] + (pos_1 / 512) % 8);
            int judge = left_size - (512 - current_running->file_list[fd].wr_pos % 512);
            int valid_bytes = (judge < 0) ? left_size : (512 - current_running->file_list[fd].wr_pos % 512);
            kmemcpy(&buffer[current_running->file_list[fd].wr_pos % 512], buff, valid_bytes);
            sbi_sd_write(kva2pa(buffer), 1, data_block_start + indirect1_p[pos_1 / 512 / 8] + (pos_1 / 512) % 8);
            left_size -= valid_bytes;
            buff += valid_bytes;
            current_running->file_list[fd].wr_pos += valid_bytes;
            if(current_running->file_list[fd].wr_pos > inode_p->bytes){
                inode_p->bytes = current_running->file_list[fd].wr_pos;
                sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
                sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
            }
        }
    }
    //need more sector
    if(left_size == 0){
        return size;
    }
    else{
        for(int i = current_running->file_list[fd].wr_pos / 512; (i < MAX_DIR) && (left_size > 0); i++){
            if(inode_p->direct[i / 8] == 0){
                inode_p->direct[i / 8] = alloc_block();
                sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
                sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
            }
            int sz = (left_size > 512) ? 512 : left_size;
            kmemcpy(buffer, buff, sz);
            sbi_sd_write(kva2pa(buffer), 1, data_block_start + inode_p->direct[i / 8] + i % 8);
            current_running->file_list[fd].wr_pos += sz;
            buff += sz;
            left_size -= sz;
            if(current_running->file_list[fd].wr_pos > inode_p->bytes){
                inode_p->bytes = current_running->file_list[fd].wr_pos;
                sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
                sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
            }
        }
    }
    //indirect1
    if(left_size <= 0){
        return size;
    }
    else{
        if(inode_p->indirect1 == 0){
            inode_p->indirect1 = alloc_block();
            sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
            sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
            sbi_sd_write(kva2pa(empty), 8, data_block_start + inode_p->indirect1);
        }
        u32 *indirect1_p = (u32 *)indirect1_space;
        sbi_sd_read(kva2pa(indirect1_p), 8, data_block_start + inode_p->indirect1);
        for(int i = current_running->file_list[fd].wr_pos - MAX_DIR * 4096; (i < 1024 * 4096) && left_size > 0; i += 512){
            if(indirect1_p[i / 512 / 8] == 0){
                indirect1_p[i / 512 / 8] = alloc_block();
                sbi_sd_write(kva2pa(indirect1_p), 8, data_block_start + inode_p->indirect1);
                sbi_sd_read(kva2pa(indirect1_p), 8, data_block_start + inode_p->indirect1);
            }
            int sz = (left_size > 512) ? 512 : left_size;
            kmemcpy(buffer, buff, sz);
            sbi_sd_write(kva2pa(buffer), 1, data_block_start + indirect1_p[i / 512 / 8] + (i / 512) % 8);
            current_running->file_list[fd].wr_pos += sz;
            buff += sz;
            left_size -= sz;
            if(current_running->file_list[fd].wr_pos > inode_p->bytes){
                inode_p->bytes = current_running->file_list[fd].wr_pos;
                sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
                sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
            }
        }
    }
    //indirect2
    if(left_size <= 0){
        return size;
    }
    else{
        if(inode_p->indirect2 == 0){
            inode_p->indirect2 = alloc_block();
            sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
            sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
            sbi_sd_write(kva2pa(empty), 8, data_block_start + inode_p->indirect2);
        }
        u32 *indirect2_p = (u32 *)indirect2_space;
        sbi_sd_read(kva2pa(indirect2_p), 8, data_block_start + inode_p->indirect2);
        for(int j = (current_running->file_list[fd].wr_pos - MAX_DIR * 4096 - 1024 * 4096) / (1024 * 4096); (j < 1024) && (left_size > 0); j++){
            if(indirect2_p[j] == 0){
                indirect2_p[j] = alloc_block();
                sbi_sd_write(kva2pa(indirect2_p), 8, data_block_start + inode_p->indirect2);
                sbi_sd_read(kva2pa(indirect2_p), 8, data_block_start + inode_p->indirect2);
                sbi_sd_write(kva2pa(empty), 8, data_block_start + indirect2_p[j]);
            }
            u32 *indirect1_p = (u32 *)indirect1_space;
            sbi_sd_read(kva2pa(indirect1_p), 8, data_block_start + indirect2_p[j]);
            for(int i = (current_running->file_list[fd].wr_pos - MAX_DIR * 4096 - 1024 * 4096) % (1024 * 4096); (i < 1024 * 4096) && (left_size > 0); i += 512){
                if(indirect1_p[i / 512 / 8] == 0){
                    indirect1_p[i / 512 / 8] = alloc_block();
                    sbi_sd_write(kva2pa(indirect1_p), 8, data_block_start + indirect2_p[j]);
                    sbi_sd_read(kva2pa(indirect1_p), 8, data_block_start + indirect2_p[j]);
                }
                int sz = (left_size > 512) ? 512 : left_size;
                kmemcpy(buffer, buff, sz);
                sbi_sd_write(kva2pa(buffer), 1, data_block_start + indirect1_p[i / 512 / 8] + (i / 512) % 8);
                current_running->file_list[fd].wr_pos += sz;
                buff += sz;
                left_size -= sz;
                if(current_running->file_list[fd].wr_pos > inode_p->bytes){
                    inode_p->bytes = current_running->file_list[fd].wr_pos;
                    sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
                    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
                }
            }
        }
    }
}

int do_lseek(int fd, int offset, int whence){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    if(whence == 0){
        current_running->file_list[fd].wr_pos = offset;
        current_running->file_list[fd].rd_pos = offset;
        return 1;
    }else if(whence == 1){
        current_running_s->file_list[fd].wr_pos += offset;
        current_running_s->file_list[fd].rd_pos += offset;
        return 1;
    }else if(whence == 2){
        u8 tmp[512];
        superblock_t *sb = (superblock_t *)tmp;
        sbi_sd_read(kva2pa(sb), 1, FS_START);
        u32 inode_block_start = sb->fs_start_block + sb->inode_block_offset;
        inode_t *inode_p = (inode_t *)tmp;
        sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
        current_running->file_list[fd].wr_pos = inode_p->bytes + offset;
        current_running->file_list[fd].rd_pos = inode_p->bytes + offset;
        return 1;
    }else{
        prints("ERROR: invalid whence\n");
        return -1;
    }
}

int relocate_wpos(int fd, int offset, int whence){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    if(whence == 0){
        current_running->file_list[fd].wr_pos = offset;
        return 1;
    }else if(whence == 1){
        current_running_s->file_list[fd].wr_pos += offset;
        return 1;
    }else if(whence == 2){
        u8 tmp[512];
        superblock_t *sb = (superblock_t *)tmp;
        sbi_sd_read(kva2pa(sb), 1, FS_START);
        u32 inode_block_start = sb->fs_start_block + sb->inode_block_offset;
        inode_t *inode_p = (inode_t *)tmp;
        sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + current_running->file_list[fd].inode);
        current_running->file_list[fd].wr_pos = inode_p->bytes + offset;
        return 1;
    }else{
        prints("ERROR: invalid whence\n");
        return -1;
    }
}


int do_link(char *src, char *dst){
    if(!check_fs()){
        prints("ERROR: no filesystem\n");
        return -1;
    }
    dentry_t dy_src;
    if(src[0] == '/'){
        dy_src = find_file(0, &src[1]);
    }else{
        dy_src = find_file(global_inode_num, &src[0]);
    }
    if(dy_src.ino == -1){
        prints("ERROR: no such file\n");
        return -1;
    }

    dentry_t dy_dst;
    if(dst[0] == '/'){
        dy_dst = find_file(0, &dst[1]);
    }else{
        dy_dst = find_file(global_inode_num, &dst[0]);
    }
    if(dy_dst.ino >= 0){
        prints("ERROR: same file has exsited\n");
        return -1;
    }

    u8 tmp[512];
    superblock_t *sb = tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    u32 fs_block_start = sb->fs_start_block;
    u32 inode_map_start = sb->inode_map_offset + fs_block_start;
    u32 block_map_start = sb->block_map_offset + fs_block_start;
    u32 inode_block_start = sb->inode_block_offset + fs_block_start;
    u32 data_block_start = sb->data_block_offset + fs_block_start;

    u8 inode_buffer[512];
    inode_t *inode_p = (inode_t *)inode_buffer;
    sbi_sd_read(kva2pa(inode_p), 1, inode_block_start + global_inode_num);
    //check space
    if(inode_p->used_size == MAX_DIR * 8){
        prints("ERROR: no space left for new directory\n");
        return -1;
    }
    inode_p->used_size++;
    inode_p->num++;
    //set new dentry
    if(inode_p->used_size % 8 == 0){
        inode_p->direct[inode_p->used_size / 8] = alloc_block();
    }
    inode_p->mtime = get_timer();
    sbi_sd_write(kva2pa(inode_p), 1, inode_block_start + global_inode_num);

    u8 dentry_buffer[512];
    kmemset(dentry_buffer, 0, 512);
    dentry_t *dy = (dentry_t *)dentry_buffer;
    dy->type = dy_src.type;
    dy->ino = dy_src.ino;
    kmemcpy(dy->name, dst, kstrlen(dst));
    sbi_sd_write(kva2pa(dy), 1, data_block_start + inode_p->direct[(inode_p->used_size - 1) / 8] + (inode_p->used_size - 1) % 8);

    int new_ino = dy->ino;
    kmemset(inode_buffer, 0, 512);
    inode_t *new_inode = (inode_t *)inode_buffer;
    sbi_sd_read(kva2pa(new_inode), 1, inode_block_start + new_ino);
    new_inode->num++;
    new_inode->mtime = get_timer();
    sbi_sd_write(kva2pa(new_inode), 1, inode_block_start + new_ino);
}

