/*
 * Copyright (C) 2010 The Android Open Source Project
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

/* A simple test of emmc random read and write performance.  When testing write
 * performance, try it twice, once with O_SYNC compiled in, and once with it commented
 * out.  Without O_SYNC, the close(2) blocks until all the dirty buffers are written
 * out, but the numbers tend to be higher.
 */

#define _LARGEFILE64_SOURCE
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

#define TST_BLK_SIZE 4096
/* Number of seconds to run the test */
#define TEST_LEN 10

static void usage(void) {
        fprintf(stderr, "Usage: rand_emmc_perf [ -r | -w ] [-o] <size_in_mb> <block_dev>\n");
        exit(1);
}

int main(int argc, char *argv[])
{
    unsigned long long max_blocks;
    int fd, fd2, write_mode = 0, iops = 0;
    struct timeval start, end, res;
    unsigned int seed;
    char buf[TST_BLK_SIZE] = { 0 };
    int c;
    int o_sync = 0;

    while ((c = getopt(argc, argv, "+rwo")) != -1) {
        switch (c) {
          case '?':
          default:
            usage();
            break;

          case 'r':
            /* Do nothing, read mode is the default */
            break;

          case 'w':
            write_mode = 1;
            break;

          case 'o':
            o_sync = O_SYNC;
            break;
        }
    }

    if (o_sync && !write_mode) {
        /* Can only specify o_sync in write mode.  Probably doesn't matter,
         * but clear o_sync if in read mode */
        o_sync = 0;
    }

    if ((argc - optind) != 2) {
        usage();
    }

    /* Size is given in megabytes, so compute the number of TST_BLK_SIZE blocks. */
    max_blocks = atol(argv[optind]) * ((1024*1024) / TST_BLK_SIZE);

    if ((fd = open(argv[optind + 1], O_RDWR | o_sync)) < 0) {
        fprintf(stderr, "Cannot open block device %s\n", argv[optind + 1]);
        exit(1);
    }

    fd2 = open("/dev/urandom", O_RDONLY);
    if (fd2 < 0) {
        fprintf(stderr, "Cannot open /dev/urandom\n");
    }
    if (read(fd2, &seed, sizeof(seed)) != sizeof(seed)) {
        fprintf(stderr, "Cannot read /dev/urandom\n");
    }
    close(fd2);
    srand(seed);

    res.tv_sec = 0;
    gettimeofday(&start, 0);
    while (res.tv_sec < TEST_LEN) {
        if (lseek64(fd, (rand() % max_blocks) * TST_BLK_SIZE, SEEK_SET) < 0) {
            fprintf(stderr, "lseek64 failed\n");
        }
        if (write_mode) {
            if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
                fprintf(stderr, "Short write\n");
            }
        } else {
            if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
                fprintf(stderr, "Short read\n");
            }
        }
        iops++;
        gettimeofday(&end, 0);
        timersub(&end, &start, &res);
    }
    close(fd);

    /* The close can take a while when in write_mode as buffers are flushed.
     * So get the time again. */
    gettimeofday(&end, 0);
    timersub(&end, &start, &res);

    printf("%d iops/sec\n", iops / (int) res.tv_sec);

    exit(0);
}

