#include <assert.h>
#include <fcntl.h>
#include <mimalloc.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    mi_option_enable(mi_option_verbose);
    mi_option_disable(mi_option_reserve_os_memory);
    mi_option_disable(mi_option_reserve_huge_os_pages);
    mi_option_enable(mi_option_limit_os_alloc);
    mi_option_enable(mi_option_disallow_os_alloc);
    mi_option_set(mi_option_arena_reserve, 0);

    size_t total_pmem_size = 1 * 1024 * 1024 * 1024ULL;
    int fd = open("/mnt/pmem1_mount/mkhalaji/mimalloc_playground/file.dat",
                  O_RDWR | O_CREAT, 0666);
    ftruncate(fd, total_pmem_size);

    void* addr = mmap(0, total_pmem_size, PROT_READ | PROT_WRITE,
                      MAP_SYNC | MAP_SHARED_VALIDATE, fd, 0);

    bool ret =
        mi_manage_os_memory((void*)addr, total_pmem_size, true, true, false, -1);

    printf("mi_manage_os_memory returned %d\n", ret);

    printf("Start of the file: %p\n", addr);

    // set up random generator: 
    srand(1234); 



    for (size_t i = 0; i < 1024 * 1024; i++) {
        void* ptr = mi_malloc(1 * 1024ULL);

        assert(ptr != NULL);

        assert(ptr >= addr && ptr < (void*)((char*)addr + total_pmem_size));

        if (rand() % 2 == 0) {
            mi_free(ptr);
        }

        if (i % 1000 == 0) {
            printf("Allocated %lu\n", i);
        }
    }

    printf("Finished\n");
    return 0;
}