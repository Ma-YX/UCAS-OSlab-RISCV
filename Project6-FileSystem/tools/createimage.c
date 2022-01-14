#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."


/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;
/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp);
static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr);
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first,int ph);
static void write_os_size(int nbytes1,int nbytes2, FILE * img);

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }
    if (argc < 3) {
        /* at least 3 args (createimage bootblock kernel) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    int ph, nbytes1 = 0,nbytes2=0, nbytes0=0,first = 1;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* open the image file */
    img = fopen(IMAGE_FILE,"wb+");
    if(img == NULL)
    printf("ERROR:Failed to open: IMAGE FILE");
    /* for each input file */
    while (nfiles-- > 0) {

        /* open input file */
        fp = fopen(*files,"r");
        if(fp == NULL)
        printf("ERROR:Failed to open input file : %d",nfiles);
        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);

        /* for each program header */
        for (ph = 0; ph < ehdr.e_phnum; ph++) {

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);

            /* write segment to the image */
            if(nfiles==0)
            write_segment(ehdr, phdr, fp, img, &nbytes1, &first,ph);
            else
            write_segment(ehdr, phdr, fp, img, &nbytes0, &first,ph);
        }
        fclose(fp);
        files++;
    }
    write_os_size(nbytes1, nbytes2,img);
    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    fread(ehdr,sizeof(Elf64_Ehdr),1,fp);
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    fseek(fp,ehdr.e_phoff+ph*sizeof(Elf64_Phdr),SEEK_SET);
    fread(phdr,sizeof(Elf64_Phdr),1,fp);
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first,int ph)
{
    char zero[1]={0};
    fseek(fp, phdr.p_offset, SEEK_SET);
    char *segment = (char *)malloc(phdr.p_filesz);
    fread(segment,phdr.p_filesz,1,fp);
    fwrite(segment,phdr.p_filesz,1,img);
    
    /*for (int i = 0; i < phdr.p_memsz - phdr.p_filesz; i++)
    {
        fwrite(zero, 1, 1, img);
    }
    (*nbytes) += phdr.p_memsz;*/
    
    (*nbytes) += phdr.p_filesz;
    if(ph==ehdr.e_phnum-1)
    if((*nbytes)%512!=0){
    int offset = 512-(*nbytes)%512;
    fwrite(zero,1,offset,img);
    (*nbytes) += offset;
    }
    
    if(options.extended == 1)
    {
    printf("\t\toffset 0x%lx\tvaddr 0x%lx\n", phdr.p_offset, phdr.p_vaddr);
    printf("\t\tfilesz 0x%lx\tmemsz 0x%lx\n", phdr.p_filesz, phdr.p_memsz);
    printf("\t\twriting 0x%lx bytes\n", phdr.p_memsz);
    printf("\t\tpadding up to 0x%x\n", *nbytes + 0x200);
    }
}

static void write_os_size(int nbytes1,int nbytes2, FILE * img)
{
    fseek(img,0x1fc,SEEK_SET);
    short block_num1 = ((nbytes1/512)*512)<nbytes1?nbytes1/512+1:nbytes1/512;
    short block_num2 = ((nbytes2/512)*512)<nbytes2?nbytes2/512+1:nbytes2/512;
    fwrite(&block_num1, 2, 1, img);
    fwrite(&block_num2, 2, 1, img);
    if(options.extended == 1)
    printf("\nos0_size: %dbytes in %dsectors\n",nbytes1,(int)block_num1);
    printf("\nos1_size: %dbytes in %dsectors\n",nbytes2,(int)block_num2);
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}



