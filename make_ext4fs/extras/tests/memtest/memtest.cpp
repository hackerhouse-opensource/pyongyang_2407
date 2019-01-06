/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/mman.h>

#if 0
const int DCACHE_SIZE = 8*1024;
const int CPU_FREQ_EST = 195;
const int BRANCH_CYCLE = 3;
#else
const int DCACHE_SIZE  = 32*1024;
const int CPU_FREQ_EST = 384;
const int BRANCH_CYCLE = 2;
#endif

//extern "C" void* xmemcpy(void*, void*, size_t);
#define MEMCPY  memcpy

typedef long long nsecs_t;

static nsecs_t system_time()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return nsecs_t(t.tv_sec)*1000000000LL + t.tv_nsec;
}

nsecs_t loop_overhead(size_t count) __attribute__((noinline));
nsecs_t loop_overhead(size_t count)
{
    nsecs_t overhead = -system_time();
    do {
        asm volatile ("":::"memory");
    } while (--count);
    overhead += system_time();
    return overhead;
}

static void preload(volatile char* addr, size_t s)
{
    for (size_t i=0 ; i<s ; i+=32) {
        char c = addr[i];
        (void)c;
    }
}

static void usage(char* p) {
    printf( "Usage: %s <test> <options>\n"
            "<test> is one of the following:\n"
            "       cpufreq\n"
            "       memcpy [perf [fast] | test]\n"
            "       memset [perf | test]\n"
            "       memcmp [perf | test]\n"
            "       strlen [perf | test]\n"
            "       malloc [fill]\n"
            "       madvise\n"
            "       resampler\n"
            "       crash\n"
            "       stack (stack smasher)\n"
            "       crawl\n"
            , p);
}

int cpufreq_test(int argc, char** argv);
int memcpy_test(int argc, char** argv);
int memset_test(int argc, char** argv);
int memcmp_test(int argc, char** argv);
int strlen_test(int argc, char** argv);
int malloc_test(int argc, char** argv);
int madvise_test(int argc, char** argv);
int crash_test(int argc, char** argv);
int stack_smasher_test(int argc, char** argv);
int crawl_test(int argc, char** argv);
int fp_test(int argc, char** argv);

#if 0
#pragma mark -
#pragma mark main
#endif

int main(int argc, char** argv)
{
    if (argc == 1) {
        usage(argv[0]);
        return 0;
    }
    int err = -1;
    if      (!strcmp(argv[1], "cpufreq"))   err = cpufreq_test(argc-1, argv+1);
    else if (!strcmp(argv[1], "memcpy"))    err = memcpy_test(argc-1, argv+1);
    else if (!strcmp(argv[1], "memset"))    err = memset_test(argc-1, argv+1);
    else if (!strcmp(argv[1], "memcmp"))    err = memcmp_test(argc-1, argv+1);
    else if (!strcmp(argv[1], "strlen"))    err = strlen_test(argc-1, argv+1);
    else if (!strcmp(argv[1], "malloc"))    err = malloc_test(argc-1, argv+1);
    else if (!strcmp(argv[1], "madvise"))   err = madvise_test(argc-1, argv+1);
    else if (!strcmp(argv[1], "crash"))     err = crash_test(argc-1, argv+1);
    else if (!strcmp(argv[1], "stack"))     err = stack_smasher_test(argc-1, argv+1);
    else if (!strcmp(argv[1], "crawl"))     err = crawl_test(argc-1, argv+1);
    else if (!strcmp(argv[1], "fp"))     err = fp_test(argc-1, argv+1);
    if (err) {
        usage(argv[0]);
    }
    return 0;
}

#if 0
#pragma mark -
#pragma mark memcpy
#endif

int validate_memcpy(char* s, char* d, size_t size);
int validate_memset(char* s, char c, size_t size);

int memcpy_test(int argc, char** argv)
{
    int option = 0;
    if (argc >= 2) {
        if (!strcmp(argv[1], "perf"))       option = 0;
        else if (!strcmp(argv[1], "test"))  option = 1;
        else                                return -1;
    }

    const int MAX_SIZE = 1024*1024; // 1MB
    const int CACHED_SPEED_EST = CPU_FREQ_EST*1024*1024; // 150 MB/s
    const int UNCACHED_SPEED_EST = (CPU_FREQ_EST/4)*1024*1024; // 60 MB/s
    char* src = (char*)malloc(MAX_SIZE+4+8+32);
    char* dst = (char*)malloc(MAX_SIZE+4+8+32);
    memset(src, 0, MAX_SIZE+4+8+32);
    memset(dst, 0, MAX_SIZE+4+8+32);

    if (option == 0) {
        bool fast = (argc>=3 && !strcmp(argv[2], "fast"));
        printf("memcpy() performance test is running, please wait...\n");
        fflush(stdout);
        usleep(10000);
        setpriority(PRIO_PROCESS, 0, -20);
        static int FAST_SIZES[] = { 1024, DCACHE_SIZE/2, DCACHE_SIZE, DCACHE_SIZE*2, MAX_SIZE };

        struct result_t { int size; float res; };
        result_t* results = (result_t*)src;
        int nbr = 0;
        int size = 0;
        for (int i=0 ; ; i++) {
            if (!fast) {
                if (size<128)          size += 8;
                else if (size<1024)    size += 128;
                else if (size<16384)   size += 1024;
                else                   size <<= 1;
            } else {
                if (size_t(i) >= sizeof(FAST_SIZES)/sizeof(FAST_SIZES[0]))
                    break;
                size = FAST_SIZES[i];
            }
            if (size > MAX_SIZE) {
                break;
            }

            const int REPEAT = (((size < DCACHE_SIZE) ?
                        (CACHED_SPEED_EST) : (UNCACHED_SPEED_EST)) / size) / 2;
                                // ~0.5 second per test

            const nsecs_t overhead = loop_overhead(REPEAT);

            // tweak to make it a bad case
            char* ddd = (char*)((long(dst+31)&~31) + 4);
            char* sss = (char*)((long(src+31)&~31) + 28);

            for (int offset=0 ; offset<=2 ; offset +=2 ) {
                memcpy(dst, src, size); // just make sure to load the caches I/D
                nsecs_t t = -system_time();
                register int count = REPEAT;
                do {
                    MEMCPY(ddd, sss+offset, size);
                } while (--count);
                t += system_time() - overhead;
                const float throughput = (size*1000000000.0f*REPEAT) / (1024*1024*t);
                results[nbr].size = size;
                results[nbr].res = throughput;
                nbr++;
            }
        }

        printf("%9s %9s %9s\n", "size", "MB/s", "MB/s (nc)");
        for (int i=0 ; i<nbr ; i+=2) {
            printf("%9d %9ld %9ld\n", results[i].size, (long)results[i].res, (long)results[i+1].res);
        }
    } else if (option == 1) {
        printf("memcpy() validation test is running, please wait...\n");
        fflush(stdout);
        char* curr = (char*)src;
        for (int i=0 ; i<MAX_SIZE ; i++) {
            char c = rand();
            *curr++ = c != 0x55 ? c : 0xAA;
        }
        char* s = src + 1024;
        char* d = dst + 1024;
        int nb = 0;
        for (int size=0 ; size<4096 && !nb ; size++) {
            nb += validate_memcpy(s, d, size);
            for (int o=1 ; o<32 && !nb ; o++) {
                nb += validate_memcpy(s+o, d, size);
                nb += validate_memcpy(s, d+o, size);
                nb += validate_memcpy(s+o, d+o, size);
            }
        }
        if (nb) printf("%d error(s) found\n", nb);
        else    printf("success!\n");
    }
    fflush(stdout);
    free(dst);
    free(src);
    return 0;
}

int validate_memcpy(char* s, char* d, size_t size)
{
    int nberr = 0;
    memset(d-4, 0x55, size+8);
    MEMCPY(s, d, size);
    if (memcmp(s,d,size)) {
        printf("*** memcpy(%p,%p,%zd) destination != source\n",s,d,size);
        nberr++;
    }
    bool r = (d[size]==0x55)&&(d[size+1]==0x55)&&(d[size+2]==0x55)&&(d[size+3]==0x55);
    if (!r) {
        printf("*** memcpy(%p,%p,%zd) clobbered past end of destination!\n",s,d,size);
        nberr++;
    }
    r = (d[-1]==0x55)&&(d[-2]==0x55)&&(d[-3]==0x55)&&(d[-4]==0x55);
    if (!r) {
        printf("*** memcpy(%p,%p,%zd) clobbered before start of destination!\n",s,d,size);
        nberr++;
    }
    return nberr;
}


#if 0
#pragma mark -
#pragma mark memset
#endif

int memset_test(int argc, char** argv)
{
    int option = 0;
    if (argc >= 2) {
        if (!strcmp(argv[1], "perf"))       option = 0;
        else if (!strcmp(argv[1], "test"))  option = 1;
        else                                return -1;
    }

    const int MAX_SIZE = 1024*1024; // 1MB
    const int CACHED_SPEED_EST = CPU_FREQ_EST*1024*1024; // 195 MB/s
    const int UNCACHED_SPEED_EST = CPU_FREQ_EST*1024*1024; // 195 MB/s
    char* dst = (char*)malloc(MAX_SIZE+4+8);

    if (option == 0) {
        printf("memset() performance test is running, please wait...\n");
        fflush(stdout);
        usleep(10000);
        setpriority(PRIO_PROCESS, 0, -20);

        static int FAST_SIZES[] = { 1024, DCACHE_SIZE/2, DCACHE_SIZE, DCACHE_SIZE*2, MAX_SIZE };
        const size_t FAST_SIZES_COUNT = sizeof(FAST_SIZES)/sizeof(FAST_SIZES[0]);
        struct result_t { int size; float res; };
        result_t results[FAST_SIZES_COUNT*2];
        int nbr = 0;
        int size = 0;
        for (int i=0 ; ; i++) {
            if (size_t(i) >= sizeof(FAST_SIZES)/sizeof(FAST_SIZES[0]))
                break;
            size = FAST_SIZES[i];
            if (size > MAX_SIZE) {
                break;
            }
            const int REPEAT = (((size < DCACHE_SIZE) ?
                        (CACHED_SPEED_EST) : (UNCACHED_SPEED_EST)) / size);
                                // ~0.5 second per test

            const nsecs_t overhead = loop_overhead(REPEAT);

            for (int j=0 ; j<2 ; j++) {
                if (j==0)   preload(dst, DCACHE_SIZE*4);   // flush D
                else        preload(dst, size);            // load D
                nsecs_t t = -system_time();
                size_t count = REPEAT;
                do {
                    memset(dst, 0, size);
                } while (--count);
                t += system_time() - overhead;

                const float throughput = (size*1000000000.0f*REPEAT) / (1024*1024*t);
                results[nbr].size = size;
                results[nbr].res = throughput;
                nbr++;
            }
        }

        printf("%9s %9s %9s\n", "size", "MB/s", "MB/s (cached)");
        for (int i=0 ; i<nbr ; i+=2) {
            printf("%9d %9ld %9ld\n", results[i].size, (long)results[i].res, (long)results[i+1].res);
        }
    } else if (option == 1) {
        printf("memset() validation test is running, please wait...\n");
        fflush(stdout);
        char* d = dst + 1024;
        int nb = 0;
        for (int o=1 ; o<32 ; o++) {
            for (int size=0 ; size<4096 && !nb ; size++) {
                nb += validate_memset(d, char(o), size);
                nb += validate_memset(d+o, char(o), size);
            }
        }
        if (nb) printf("%d error(s) found\n", nb);
        else    printf("success!\n");
    }
    fflush(stdout);
    free(dst);
    return 0;
}

int validate_memset(char* d, char c, size_t size)
{
    int nberr = 0;
    for (size_t i=0; i<size ; d[i++]=0xaa) ;
    d[-1] = 0x55;
    d[size+1] = 0x55;
    memset(d, c, size);
    if (d[size+1]!=0x55) {
        printf("*** memset(%p,%02x,%zd) clobbered past end of destination!\n",d,(int)c,size);
        nberr++;
    }
    if (d[-1]!=0x55) {
        printf("*** memset(%p,%02x,%zd) clobbered before start of destination!\n",d,(int)c,size);
        nberr++;
    }
    for (size_t i=0 ; i<size ; i++) {
        if (d[i] != c) {
            printf("*** memset(%p,%02x,%zd) failed at offset %zd\n",d,(int)c,size, i);
            nberr++;
            break;
        }
    }
    return nberr;
}

#if 0
#pragma mark -
#pragma mark memcmp
#endif

static int ref_memcmp(const void *s1, const void *s2, size_t n)
{
  const unsigned char *c1 = (const unsigned char *)s1, *c2 = (const unsigned char *)s2;
  int d = 0;

  while ( n-- ) {
    d = (int)*c1++ - (int)*c2++;
    if ( d )
      break;
  }

  return (d < 0 ? -1 : (d > 0 ? 1 : 0));
}

int validate_memcmp(const char* s, const char* d, size_t size)
{

    int a = ref_memcmp(s, d, size);
    int b = memcmp(s, d, size);
    b = (b < 0 ? -1 : (b > 0 ? 1 : 0));
    //printf("%d, %d\n", a, b);
    if (a != b) {
        printf("*** memcmp(%p,%p,%zd) failed %d should be %d\n",s,d,size,b,a);
        return 1;
    }
    return 0;
}

int memcmp_test(int argc, char** argv)
{
    int option = 0;
    if (argc >= 2) {
        if (!strcmp(argv[1], "perf"))       option = 0;
        else if (!strcmp(argv[1], "test"))  option = 1;
        else                                return -1;
    }

    const int MAX_SIZE = 1024*1024; // 1MB
    const int CACHED_SPEED_EST = CPU_FREQ_EST*1024*1024; // 150 MB/s
    const int UNCACHED_SPEED_EST = (CPU_FREQ_EST/4)*1024*1024; // 60 MB/s
    char* src = (char*)malloc(MAX_SIZE+4+8+32);
    char* dst = (char*)malloc(MAX_SIZE+4+8+32);

    if (option == 0) {
        printf("memcmp() performance test is running, please wait...\n");
        fflush(stdout);
        usleep(10000);
        setpriority(PRIO_PROCESS, 0, -20);

        static int FAST_SIZES[] = { 1024, DCACHE_SIZE/2, DCACHE_SIZE, DCACHE_SIZE*2, MAX_SIZE };

        struct result_t { int size; float res; };
        result_t* results = (result_t*)src;
        int nbr = 0;
        int size = 0;
        for (int i=0 ; ; i++) {
            if (size_t(i) >= sizeof(FAST_SIZES)/sizeof(FAST_SIZES[0]))
                break;
            size = FAST_SIZES[i];
            if (size > MAX_SIZE) {
                break;
            }

            const int REPEAT = (((size < DCACHE_SIZE) ?
                        (CACHED_SPEED_EST) : (UNCACHED_SPEED_EST)) / size) / 2;
                                // ~0.5 second per test

            const nsecs_t overhead = loop_overhead(REPEAT);

            // tweak to make it a bad case
            char* ddd = (char*)((long(dst+31)&~31) + 4);
            char* sss = (char*)((long(src+31)&~31) + 28);

            for (int offset=0 ; offset<=2 ; offset +=2 ) {
                memcpy(ddd, sss+offset, size); // just make sure to load the caches I/D
                nsecs_t t = -system_time();
                register int count = REPEAT;
                char c;
                c = memcmp(ddd, sss+offset, size);
                //printf("size %d, memcmp -> %d\n", size, (int)c);
                do {
                    c = memcmp(ddd, sss+offset, size);
                    asm volatile (""::"r"(c):"memory");
                } while (--count);
                t += system_time() - overhead;
                const float throughput = (size*1000000000.0f*REPEAT) / (1024*1024*t);
                results[nbr].size = size;
                results[nbr].res = throughput;
                nbr++;
            }
        }

        printf("%9s %9s %9s\n", "size", "MB/s", "MB/s (nc)");
        for (int i=0 ; i<nbr ; i+=2) {
            printf("%9d %9ld %9ld\n", results[i].size, (long)results[i].res, (long)results[i+1].res);
        }
    } else {
        printf("memcmp() validation test is running, please wait...\n");
        fflush(stdout);

        const char* const s = (const char*)src + 1024;
        const char* const d = (const char*)dst + 1024;
        int nb = 0;
        for (int j=0 ; j<32 ; j++) {

            char *curr0 = (char*)src;
            char *curr1 = (char*)dst;
            for (int i=0 ; i<MAX_SIZE ; i++) {
                char c = rand();
                *curr0++ = c;
                *curr1++ = c;
            }
            if (j) {
                src[1024 + j] ^= 0xFF;
            }


            for (int size=0 ; size<32 && !nb ; size++) {
                for (int o=0 ; o<4 ; o++) {
                    nb += validate_memcmp(s+o, d+o, size);
                }
               // memmove((char*)d+1, d, size);
                for (int o=0 ; o<4 ; o++) {
                    nb += validate_memcmp(s, d+o, size);
                }
            }
        }
        if (nb) printf("%d error(s) found\n", nb);
        else    printf("success!\n");
    }
    fflush(stdout);
    free(dst);
    free(src);
    return 0;
}

#if 0
#pragma mark -
#pragma mark strlen
#endif

int strlen_test(int argc, char** argv)
{
    int option = 0;
    if (argc >= 2) {
        if (!strcmp(argv[1], "perf"))       option = 0;
        else if (!strcmp(argv[1], "test"))  option = 1;
        else                                return -1;
    }

    const int MAX_SIZE = 1024*1024; // 1MB
    const int CACHED_SPEED_EST = CPU_FREQ_EST*1024*1024; // 195 MB/s
    const int UNCACHED_SPEED_EST = CPU_FREQ_EST*1024*1024; // 195 MB/s
    char* str = (char*)calloc(MAX_SIZE+4+8, 1);

    if (option == 0) {
        printf("strlen() performance test is running, please wait...\n");
        fflush(stdout);
        usleep(10000);
        setpriority(PRIO_PROCESS, 0, -20);

        static int FAST_SIZES[] = { 1024, DCACHE_SIZE/2, DCACHE_SIZE, DCACHE_SIZE*2, MAX_SIZE };
        const size_t FAST_SIZES_COUNT = sizeof(FAST_SIZES)/sizeof(FAST_SIZES[0]);
        struct result_t { int size; float res; };
        result_t results[FAST_SIZES_COUNT*2];
        int nbr = 0;
        int size = 0;
        for (int i=0 ; ; i++) {
            if (size_t(i) >= sizeof(FAST_SIZES)/sizeof(FAST_SIZES[0]))
                break;
            size = FAST_SIZES[i];
            if (size > MAX_SIZE) {
                break;
            }
            const int REPEAT = (((size < DCACHE_SIZE) ?
                        (CACHED_SPEED_EST) : (UNCACHED_SPEED_EST)) / size);
                                // ~0.5 second per test

            const nsecs_t overhead = loop_overhead(REPEAT);

            for (int j=0 ; j<2 ; j++) {
                memset(str, 'A', size-1);
                if (j==0)   preload(str, DCACHE_SIZE*4);   // flush D
                else        preload(str, size);            // load D

                nsecs_t t = -system_time();
                size_t count = REPEAT;
                int c=0;
                do {
                    c = strlen(str);
                    asm volatile (""::"r"(c):"memory");
                } while (--count);
                t += system_time() - overhead;

                const float throughput = (size*1000000000.0f*REPEAT) / (1024*1024*t);
                results[nbr].size = size;
                results[nbr].res = throughput;
                nbr++;
            }
        }

        printf("%9s %9s %9s\n", "size", "MB/s", "MB/s (cached)");
        for (int i=0 ; i<nbr ; i+=2) {
            printf("%9d %9ld %9ld\n", results[i].size, (long)results[i].res, (long)results[i+1].res);
        }
    }

    fflush(stdout);
    free(str);
    return 0;
}


#if 0
#pragma mark -
#pragma mark malloc
#endif

int malloc_test(int argc, char** argv)
{
    bool fill = (argc>=2 && !strcmp(argv[1], "fill"));
    size_t total = 0;
    size_t size = 0x40000000;
    while (size) {
        void* addr = malloc(size);
        if (addr == 0) {
            printf("size = %9zd failed\n", size);
            size >>= 1;
        } else {
            total += size;
            printf("size = %9zd, addr = %p (total = %9zd (%zd MB))\n",
                    size, addr, total, total / (1024*1024));
            if (fill) {
                printf("filling...\n");
                fflush(stdout);
                memset(addr, 0, size);
            }
            size = size + (size>>1);
        }
    }
    printf("done. allocated %zd MB\n", total / (1024*1024));
    return 0;
}

#if 0
#pragma mark -
#pragma mark madvise
#endif

int madvise_test(int argc, char** argv)
{
    for (int i=0 ; i<2 ; i++) {
        size_t size = i==0 ? 4096 : 48*1024*1024; // 48 MB
        printf("Allocating %zd MB... ", size/(1024*1024)); fflush(stdout);
        void* addr1 = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        printf("%p (%s)\n", addr1, addr1==(void*)-1 ? "failed" : "OK"); fflush(stdout);

        printf("touching %p...\n", addr1); fflush(stdout);
        memset(addr1, 0x55, size);

        printf("advising DONTNEED...\n"); fflush(stdout);
        madvise(addr1, size, MADV_DONTNEED);

        printf("reading back %p...\n", addr1); fflush(stdout);
        if (*(long*)addr1 == 0) {
            printf("madvise freed some pages\n");
        } else if (*(long*)addr1 == 0x55555555) {
            printf("pages are still there\n");
        } else {
            printf("getting garbage back\n");
        }

        printf("Allocating %zd MB... ", size/(1024*1024)); fflush(stdout);
        void* addr2 = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        printf("%p (%s)\n", addr2, addr2==(void*)-1 ? "failed" : "OK"); fflush(stdout);

        printf("touching %p...\n", addr2); fflush(stdout);
        memset(addr2, 0xAA, size);

        printf("unmap %p ...\n", addr2); fflush(stdout);
        munmap(addr2, size);

        printf("touching %p...\n", addr1); fflush(stdout);
        memset(addr1, 0x55, size);

        printf("unmap %p ...\n", addr1); fflush(stdout);
        munmap(addr1, size);
    }

    printf("Done\n"); fflush(stdout);
    return 0;
}

#if 0
#pragma mark -
#pragma mark cpufreq
#endif

int cpufreq_test(int argc, char** argv)
{
    struct timespec res;
    clock_getres(CLOCK_REALTIME, &res);
    printf("CLOCK_REALTIME  resolution: %lu ns\n", res.tv_nsec);
    clock_getres(CLOCK_MONOTONIC, &res);
    printf("CLOCK_MONOTONIC resolution: %lu ns\n", res.tv_nsec);
    clock_getres(CLOCK_PROCESS_CPUTIME_ID, &res);
    printf("CLOCK_PROCESS_CPUTIME_ID resolution: %lu ns\n", res.tv_nsec);
    clock_getres(CLOCK_THREAD_CPUTIME_ID, &res);
    printf("CLOCK_THREAD_CPUTIME_ID  resolution: %lu ns\n", res.tv_nsec);

    if (clock_getres(CLOCK_REALTIME_HR, &res) != 0)
        printf("CLOCK_REALTIME_HR   resolution: %lu ns\n", res.tv_nsec);
    else
        printf("CLOCK_REALTIME_HR   not supported\n");

    if (clock_getres(CLOCK_MONOTONIC_HR, &res) != 0)
        printf("CLOCK_MONOTONIC_HR  resolution: %lu ns\n", res.tv_nsec);
    else
        printf("CLOCK_MONOTONIC_HR  not supported\n");

    printf("\nEstimating the CPU frequency, please wait...\n");
    fflush(stdout);
    usleep(10000);
    setpriority(PRIO_PROCESS, 0, -20);

    const int LOOP_CYCLES = 1+BRANCH_CYCLE; // 1 cycle + 3 cycles for the branch
    const size_t REPEAT = CPU_FREQ_EST*1000000;   // ~4 seconds (4cycles/loop)
    register size_t count = REPEAT;
    nsecs_t t = system_time();
    do { // this loop generates 1+3 cycles
        asm volatile ("":::"memory");
    } while (--count);
    t = system_time() - t;
    const float freq = t ? (1000.0f*float(REPEAT)*LOOP_CYCLES) / t : 0;
    printf("this CPU frequency: %ld MHz\n", long(freq+0.5f));
    return 0;
}

#if 0
#pragma mark -
#pragma mark crash_test
#endif

int crash_test(int argc, char** argv)
{
    printf("about to crash...\n");
    asm volatile(
        "mov r0,  #0 \n"
        "mov r1,  #1 \n"
        "mov r2,  #2 \n"
        "mov r3,  #3 \n"
        "ldr r12, [r0] \n"
    );

    return 0;
}

int stack_smasher_test(int argc, char** argv)
{
    int dummy = 0;
    printf("corrupting our stack...\n");
    *(volatile long long*)&dummy = 0;
    return 0;
}

// --------------------------------------------------------------------

extern "C" void thumb_function_1(int*p);
extern "C" void thumb_function_2(int*p);
extern "C" void arm_function_3(int*p);
extern "C" void arm_function_2(int*p);
extern "C" void arm_function_1(int*p);

void arm_function_3(int*p) {
    int a = 0;
    thumb_function_2(&a);
}

void arm_function_2(int*p) {
    int a = 0;
    thumb_function_1(&a);
}

void arm_function_1(int*p) {
    int a = 0;
    arm_function_2(&a);
}

int crawl_test(int argc, char** argv)
{
    int a = 0;
    arm_function_1(&a);
    return 0;
}

