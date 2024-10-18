#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <mimalloc.h>
#include "mimalloc/internal.h"
#include "mimalloc/atomic.h"
#include "mimalloc/prim.h"   // _mi_prim_thread_id()

#define THREAD_COUNT 24

size_t total_pmem_size = 1 * 1024 * 1024 * 1024ULL;
void* addr = NULL;

void* thread_func(const int tid) {
    mi_heap_t* defaultHeap = mi_heap_get_default();
    int garbage = 0;
    for (int i = 0; i < (THREAD_COUNT - tid) * 1000000; i++) {
        garbage += i / 2;
    }
    printf("Thread=%d, defaultHeap=%p\n", tid, defaultHeap);
    for (int i = 0; i < 100; i++) { 
        void *p = mi_malloc(2240);
        printf("Thread=%d, allocated=%p\n", tid, p);
    }
}

int main() {
    mi_option_enable(mi_option_verbose);
    mi_option_disable(mi_option_reserve_os_memory);
    mi_option_disable(mi_option_reserve_huge_os_pages);
    mi_option_enable(mi_option_limit_os_alloc);

    mi_option_enable(mi_option_disallow_os_alloc);
    mi_option_set(mi_option_arena_reserve, 0);


    // OPEN AND TRUNCATE THE FILE
    int fd = open("/mnt/pmem1_mount/mkhalaji/mimalloc_playground/file.dat",
                  O_RDWR | O_CREAT, 0666);
    ftruncate(fd, total_pmem_size);

    // MMAP THE FILE
    addr = mmap(0, total_pmem_size, PROT_READ | PROT_WRITE,
                MAP_SYNC | MAP_SHARED_VALIDATE, fd, 0);

    printf("Start of the file: %p\n", addr); 


    bool ret = mi_manage_os_memory((void*)addr, total_pmem_size, true, false,
                                   false, -1);

    printf("mi_manage_os_memory returned %d\n", ret);


    void *p = mi_malloc(2048);

    printf("Allocated: %p\n", p);


    pthread_t threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, thread_func, i);
    }
    // wait for all threads to finish
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Finished\n");
    return 0;
}