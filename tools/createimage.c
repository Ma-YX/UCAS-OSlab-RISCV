#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define OS_SIZE_LOC 0x1fc

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
                          FILE * img, int *nbytes, int *first);
static void write_os_size(int nbytes, FILE * img);

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
    int ph, nbytes = 0, first = 1;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* open the image file */
    if((img = fopen(IMAGE_FILE, "wb+")) == NULL){
        printf("Cannnot open image file\n");
        return;
    }
    /* for each input file */
    while (nfiles-- > 0) {

        /* open input file */
        if((fp = fopen(*files, "r")) == NULL){
            printf("Error: cannot open input file\n");
            return;
        }
        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);

        /* for each program header */
        for (ph = 0; ph < ehdr.e_phnum; ph++) {
            if(options.extended == 1){
                printf("\tsegmemt %d\n", ph);
            }
            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);

            /* write segment to the image */
            write_segment(ehdr, phdr, fp, img, &nbytes, &first);
        }
        fclose(fp);
        files++;
    }
    write_os_size(nbytes, img);
    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    fread(ehdr, sizeof(Elf64_Ehdr), 1, fp);
    return;
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    long ofs;
    ofs = ehdr.e_phoff + ph * sizeof(Elf64_Phdr);
    fseek(fp, ofs, SEEK_SET);
    fread(phdr, sizeof(Elf64_Phdr), 1, fp);
    return;
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first)
{
    //read segmemt and add '0'
    char *seg;
    int size;
    size = ((phdr.p_memsz + 511) / 512) * 512;
    seg = (char *)malloc(sizeof(char)*size);
    fseek(fp, phdr.p_offset, SEEK_SET);
    fread(seg, phdr.p_filesz, 1, fp);

    //write segment of bootlock or kernel
    if(*first){
        //bootblock
        fwrite(seg, (long)(size), 1, img);
        *first = 0;
    }else{
        //kernel
        fwrite(seg, (long)(size), 1, img);
        *nbytes += size;
    }
    //print infomation of segment
    printf("\t\toffset 0x%lx\tvaddr 0x%lx\n", phdr.p_offset, phdr.p_vaddr);
    printf("\t\tfilesz 0x%lx\tmemsz 0x%lx\n", phdr.p_filesz, phdr.p_memsz);
    printf("\t\twriting 0x%lx bytes\n", phdr.p_memsz);
    printf("\t\tpadding up to 0x%x\n", *nbytes + 0x200);
    return;
}

static void write_os_size(int nbytes, FILE * img)
{
    short kernel_size;
    kernel_size = nbytes / 512;
    fseek(img, OS_SIZE_LOC, SEEK_SET);
    fwrite(&kernel_size, 2, 1, img);
    if(options.extended == 1){
        printf("os_size: %d sectors\n", kernel_size);
    }
    return;
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
