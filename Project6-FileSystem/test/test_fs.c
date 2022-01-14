#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#include <os.h>

static char buff[64];

int main(void)
{
    int i, j;
    int fd = sys_fopen("1.txt", O_RDWR);
    
    printf("START BIG FILE TEST!!!\n");
    printf("[PART 1] Write to:%dBytes\n",0);
    for (i = 0; i < 3; i++)
    {
        sys_fwrite(fd, "hello world!\n", 13);
    }
    for (i = 0; i < 3; i++)
    {
        sys_fread(fd, buff, 13);
        for (j = 0; j < 13; j++)
        {
            printf("%c", buff[j]);
        }
    }
    printf("-----------------------PART 1 END---------------------\n",0);
    
    printf("\n[PART 2] Write to:%dBytes\n",40*1024+39);
    sys_lseek(fd,40*1024,1);
    char *cat[6];
    cat[0] = "This is part 2 of test for large file.\n";
    cat[1] = "The second part also test mode 1 of lseek.\n";
    cat[2] = "Again:\n";
    cat[3] = "hello world!\n";
    cat[4] = "-----------------------PART 2 END---------------------\n";
    for(i = 0;i<5;++i)
    sys_fwrite(fd, cat[i], strlen(cat[i]));
    for(i = 0;i<5;++i){
    sys_fread(fd, buff, strlen(cat[i]));
    for(int j =0;j<strlen(cat[i]);++j)
    printf("%c",buff[j]);
    }

    printf("\n[PART 3] Write to:%dBytes\n",16*1024*1024+40+40*1024+39+59);
    sys_lseek(fd,16*1024*1024+40,2);
    char *dog[6];
    dog[0] = "This is part 3 of test for large file.\n";
    dog[1] = "The second part also test mode 2 of lseek.\n";
    dog[2] = "Again and again:\n";
    dog[3] = "hello world!\n";
    dog[4] = "-----------------------PART 3 END---------------------\n";
    for(i = 0;i<5;++i)
    sys_fwrite(fd, dog[i], strlen(dog[i]));
    for(i = 0;i<5;++i){
    sys_fread(fd, buff, strlen(dog[i]));
    for(int j =0;j<strlen(dog[i]);++j)
    printf("%c",buff[j]);
    }

    printf("----------------------------TEST END-----------------------------\n");
    sys_close(fd);
}