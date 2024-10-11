#include <fcntl.h>
#include <mimalloc.h>
#include <mimalloc/types.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MB * 1024 * 1024
#define FILE_SIZE (256 MB)  // 256MB


const char *text = "Hello, World!";


int main() {
    mi_option_enable(mi_option_verbose);
    mi_option_disable(mi_option_reserve_os_memory);
    mi_option_disable(mi_option_reserve_huge_os_pages);
    mi_option_enable(mi_option_limit_os_alloc); 

    const char *filename = "/home/mohammad/repos/mimalloc/mmapped_file.txt";
    int fd;

    printf("Creating a file with size %d\n", FILE_SIZE);
    fd = open(filename, O_RDWR | O_CREAT, 0666);

    if (fd == -1) {
        perror("Error opening file for writing");
        return 1;
    }

    if (ftruncate(fd, FILE_SIZE) != 0) {
        perror("Error truncating file");
        return 1;
    }


    printf("mmapping the file\n");
    // make sure to align the mmapped memory to a multiple of the page size: MI_SEGMENT_SIZE

    char *file_in_memory = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_32BIT, fd, 0);
    if (file_in_memory == MAP_FAILED) {
        perror("Error mmapping the file");
        return 1;
    }

    close(fd);


    memset(file_in_memory, (char)0, FILE_SIZE);
    msync(file_in_memory, FILE_SIZE, MS_SYNC);

    uintptr_t start = (uintptr_t)file_in_memory;
    if (start % MI_SEGMENT_ALIGN != 0) {
        printf("start is not aligned to MI_SEGMENT_ALIGN\n");
        return 1;
    }

    printf("mmap successful, passing it as an arena to mimalloc\n");


    bool ret = mi_manage_os_memory((void *)file_in_memory, FILE_SIZE, true, true, true, -1);


    void *p = mi_malloc(1 MB);
    void *q = mi_malloc(1 MB);
    void *r = mi_malloc(1 MB);
    void *s = mi_malloc(1 MB);

    printf("base: %p\n", file_in_memory);
    printf("p   : %p\n", p);
    printf("q   : %p\n", q);
    printf("r   : %p\n", r);
    printf("s   : %p\n", s);

    // am I writing to the file or is mimalloc getting memory from elsewhere?
    memcpy(p, text, strlen(text));

    printf("freeing the memory\n");

    int offset = (size_t) p - (size_t) file_in_memory;
    for (int i = offset; i < offset + strlen(text); i++) {
        printf("i: %d, buf[i]: %c\n", i, file_in_memory[i]);
    }

    // mi_free(p);
    // mi_free(q);
    // mi_free(r);
    // mi_free(s);

    printf("Writing to the file\n");
    msync(file_in_memory, FILE_SIZE, MS_SYNC);

    printf("Unmapping the file\n");
    // munmap(file_in_memory, FILE_SIZE);

    // open the file again and check if the data is still there
    printf("Reading the file\n");
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for reading");
        return 1;
    }

    // print the bytes you wrote to the file

    char *buf = (char *)malloc(FILE_SIZE);
    if (read(fd, buf, FILE_SIZE) == -1) {
        perror("Error reading from file");
        return 1;
    }

    // print bytes from p to p+strlen(text)
    printf("Printing the bytes from the file\n");


    for (int i = offset; i < offset + strlen(text); i++) {
        printf("i: %d, buf[i]: %c\n", i, buf[i]);
    }
    printf("\n");


    return 0;
}