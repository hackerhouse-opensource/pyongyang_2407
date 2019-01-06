/*
** Copyright 2010 The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*
 * Micro-benchmarking of sleep/cpu speed/memcpy/memset/memory reads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <sched.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

// The default size of data that will be manipulated in each iteration of
// a memory benchmark. Can be modified with the --data_size option.
#define DEFAULT_DATA_SIZE       1000000000

// Number of nanoseconds in a second.
#define NS_PER_SEC              1000000000

// The maximum number of arguments that a benchmark will accept.
#define MAX_ARGS    2

// Use macros to compute values to try and avoid disturbing memory as much
// as possible after each iteration.
#define COMPUTE_AVERAGE_KB(avg_kb, bytes, time_ns) \
        avg_kb = ((bytes) / 1024.0) / ((double)(time_ns) / NS_PER_SEC);

#define COMPUTE_RUNNING(avg, running_avg, square_avg, cur_idx) \
    running_avg = ((running_avg) / ((cur_idx) + 1)) * (cur_idx) + (avg) / ((cur_idx) + 1); \
    square_avg = ((square_avg) / ((cur_idx) + 1)) * (cur_idx) + ((avg) / ((cur_idx) + 1)) * (avg);

#define GET_STD_DEV(running_avg, square_avg) \
    sqrt((square_avg) - (running_avg) * (running_avg))

// Contains information about benchmark options.
typedef struct {
    bool print_average;
    bool print_each_iter;

    int dst_align;
    int src_align;

    int cpu_to_lock;

    int data_size;

    int args[MAX_ARGS];
    int num_args;
} command_data_t;

// Struct that contains a mapping of benchmark name to benchmark function.
typedef struct {
    const char *name;
    int (*ptr)(const command_data_t &cmd_data);
} function_t;

// Get the current time in nanoseconds.
uint64_t nanoTime() {
  struct timespec t;

  t.tv_sec = t.tv_nsec = 0;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return static_cast<uint64_t>(t.tv_sec) * NS_PER_SEC + t.tv_nsec;
}

// Allocate memory with a specific alignment and return that pointer.
// This function assumes an alignment value that is a power of 2.
// If the alignment is 0, then use the pointer returned by malloc.
uint8_t *allocateAlignedMemory(size_t size, int alignment) {
  uint64_t ptr = reinterpret_cast<uint64_t>(malloc(size + 2 * alignment));
  if (!ptr)
      return NULL;
  if (alignment > 0) {
      // When setting the alignment, set it to exactly the alignment chosen.
      // The pointer returned will be guaranteed not to be aligned to anything
      // more than that.
      ptr += alignment - (ptr & (alignment - 1));
      ptr |= alignment;
  }

  return reinterpret_cast<uint8_t*>(ptr);
}

int benchmarkSleep(const command_data_t &cmd_data) {
    uint64_t time_ns;

    int delay = cmd_data.args[0];
    int iters = cmd_data.args[1];
    bool print_each_iter = cmd_data.print_each_iter;
    bool print_average = cmd_data.print_average;
    double avg, running_avg = 0.0, square_avg = 0.0;
    for (int i = 0; iters == -1 || i < iters; i++) {
        time_ns = nanoTime();
        sleep(delay);
        time_ns = nanoTime() - time_ns;

        avg = (double)time_ns / NS_PER_SEC;

        if (print_average) {
            COMPUTE_RUNNING(avg, running_avg, square_avg, i);
        }

        if (print_each_iter) {
            printf("sleep(%d) took %.06f seconds\n", delay, avg);
        }
    }

    if (print_average) {
        printf("  sleep(%d) average %.06f seconds std dev %f\n", delay,
               running_avg, GET_STD_DEV(running_avg, square_avg));
    }

    return 0;
}

int benchmarkCpu(const command_data_t &cmd_data) {
    // Use volatile so that the loop is not optimized away by the compiler.
    volatile int cpu_foo;

    uint64_t time_ns;
    int iters = cmd_data.args[1];
    bool print_each_iter = cmd_data.print_each_iter;
    bool print_average = cmd_data.print_average;
    double avg, running_avg = 0.0, square_avg = 0.0;
    for (int i = 0; iters == -1 || i < iters; i++) {
        time_ns = nanoTime();
        for (cpu_foo = 0; cpu_foo < 100000000; cpu_foo++);
        time_ns = nanoTime() - time_ns;

        avg = (double)time_ns / NS_PER_SEC;

        if (print_average) {
            COMPUTE_RUNNING(avg, running_avg, square_avg, i);
        }

        if (print_each_iter) {
            printf("cpu took %.06f seconds\n", avg);
        }
    }

    if (print_average) {
        printf("  cpu average %.06f seconds std dev %f\n",
               running_avg, GET_STD_DEV(running_avg, square_avg));
    }

    return 0;
}

int benchmarkMemset(const command_data_t &cmd_data) {
    int size = cmd_data.args[0];
    int iters = cmd_data.args[1];

    uint8_t *dst = allocateAlignedMemory(size, cmd_data.dst_align);
    if (!dst)
        return -1;

    double avg_kb, running_avg_kb = 0.0, square_avg_kb = 0.0;
    uint64_t time_ns;
    int j;
    bool print_average = cmd_data.print_average;
    bool print_each_iter = cmd_data.print_each_iter;
    int copies = cmd_data.data_size/size;
    for (int i = 0; iters == -1 || i < iters; i++) {
        time_ns = nanoTime();
        for (j = 0; j < copies; j++)
            memset(dst, 0, size);
        time_ns = nanoTime() - time_ns;

        // Compute in kb to avoid any overflows.
        COMPUTE_AVERAGE_KB(avg_kb, copies * size, time_ns);

        if (print_average) {
            COMPUTE_RUNNING(avg_kb, running_avg_kb, square_avg_kb, i);
        }

        if (print_each_iter) {
            printf("memset %dx%d bytes took %.06f seconds (%f MB/s)\n",
                   copies, size, (double)time_ns / NS_PER_SEC, avg_kb / 1024.0);
        }
    }

    if (print_average) {
        printf("  memset %dx%d bytes average %.2f MB/s std dev %.4f\n",
               copies, size, running_avg_kb / 1024.0,
               GET_STD_DEV(running_avg_kb, square_avg_kb) / 1024.0);
    }
    return 0;
}

int benchmarkMemcpy(const command_data_t &cmd_data) {
    int size = cmd_data.args[0];
    int iters = cmd_data.args[1];

    uint8_t *src = allocateAlignedMemory(size, cmd_data.src_align);
    if (!src)
        return -1;
    uint8_t *dst = allocateAlignedMemory(size, cmd_data.dst_align);
    if (!dst)
        return -1;

    uint64_t time_ns;
    double avg_kb, running_avg_kb = 0.0, square_avg_kb = 0.0;
    int j;
    bool print_average = cmd_data.print_average;
    bool print_each_iter = cmd_data.print_each_iter;
    int copies = cmd_data.data_size / size;
    for (int i = 0; iters == -1 || i < iters; i++) {
        time_ns = nanoTime();
        for (j = 0; j < copies; j++)
            memcpy(dst, src, size);
        time_ns = nanoTime() - time_ns;

        // Compute in kb to avoid any overflows.
        COMPUTE_AVERAGE_KB(avg_kb, copies * size, time_ns);

        if (print_average) {
            COMPUTE_RUNNING(avg_kb, running_avg_kb, square_avg_kb, i);
        }

        if (print_each_iter) {
            printf("memcpy %dx%d bytes took %.06f seconds (%f MB/s)\n",
                   copies, size, (double)time_ns / NS_PER_SEC, avg_kb / 1024.0);
        }
    }
    if (print_average) {
        printf("  memcpy %dx%d bytes average %.2f MB/s std dev %.4f\n",
               copies, size, running_avg_kb/1024.0,
               GET_STD_DEV(running_avg_kb, square_avg_kb) / 1024.0);
    }
    return 0;
}

int benchmarkMemread(const command_data_t &cmd_data) {
    int size = cmd_data.args[0];
    int iters = cmd_data.args[1];

    int *src = reinterpret_cast<int*>(malloc(size));
    if (!src)
        return -1;

    // Use volatile so the compiler does not optimize away the reads.
    volatile int foo;
    uint64_t time_ns;
    int j, k;
    double avg_kb, running_avg_kb = 0.0, square_avg_kb = 0.0;
    bool print_average = cmd_data.print_average;
    bool print_each_iter = cmd_data.print_each_iter;
    int c = cmd_data.data_size / size;
    for (int i = 0; iters == -1 || i < iters; i++) {
        time_ns = nanoTime();
        for (j = 0; j < c; j++)
            for (k = 0; k < size/4; k++)
                foo = src[k];
        time_ns = nanoTime() - time_ns;

        // Compute in kb to avoid any overflows.
        COMPUTE_AVERAGE_KB(avg_kb, c * size, time_ns);

        if (print_average) {
            COMPUTE_RUNNING(avg_kb, running_avg_kb, square_avg_kb, i);
        }

        if (print_each_iter) {
            printf("read %dx%d bytes took %.06f seconds (%f MB/s)\n",
                   c, size, (double)time_ns / NS_PER_SEC, avg_kb / 1024.0);
        }
    }

    if (print_average) {
        printf("  read %dx%d bytes average %.2f MB/s std dev %.4f\n",
               c, size, running_avg_kb/1024.0,
               GET_STD_DEV(running_avg_kb, square_avg_kb) / 1024.0);
    }

    return 0;
}

// Create the mapping structure.
function_t function_table[] = {
    { "sleep", benchmarkSleep },
    { "cpu", benchmarkCpu },
    { "memset", benchmarkMemset },
    { "memcpy", benchmarkMemcpy },
    { "memread", benchmarkMemread },
    { NULL, NULL }
};

void usage() {
    printf("Usage:\n");
    printf("  micro_bench [--data_size DATA_BYTES] [--print_average]\n");
    printf("              [--no_print_each_iter] [--lock_to_cpu CORE]\n");
    printf("    --data_size DATA_BYTES\n");
    printf("      For the data benchmarks (memcpy/memset/memread) the approximate\n");
    printf("      size of data, in bytes, that will be manipulated in each iteration.\n");
    printf("    --print_average\n");
    printf("      Print the average and standard deviation of all iterations.\n");
    printf("    --no_print_each_iter\n");
    printf("      Do not print any values in each iteration.\n");
    printf("    --lock_to_cpu CORE\n");
    printf("      Lock to the specified CORE. The default is to use the last core found.\n");
    printf("    ITERS\n");
    printf("      The number of iterations to execute each benchmark. If not\n");
    printf("      passed in then run forever.\n");
    printf("  micro_bench sleep TIME_TO_SLEEP [ITERS]\n");
    printf("    TIME_TO_SLEEP\n");
    printf("      The time in seconds to sleep.\n");
    printf("  micro_bench cpu UNUSED [ITERS]\n");
    printf("  micro_bench [--dst_align ALIGN] memset NUM_BYTES [ITERS]\n");
    printf("    --dst_align ALIGN\n");
    printf("      Align the memset destination pointer to ALIGN. The default is to use the\n");
    printf("      value returned by malloc.\n");
    printf("  micro_bench [--src_align ALIGN] [--dst_align ALIGN] memcpy NUM_BYTES [ITERS]\n");
    printf("    --src_align ALIGN\n");
    printf("      Align the memcpy source pointer to ALIGN. The default is to use the\n");
    printf("      value returned by malloc.\n");
    printf("    --dst_align ALIGN\n");
    printf("      Align the memcpy destination pointer to ALIGN. The default is to use the\n");
    printf("      value returned by malloc.\n");
    printf("  micro_bench memread NUM_BYTES [ITERS]\n");
}

function_t *processOptions(int argc, char **argv, command_data_t *cmd_data) {
    function_t *command = NULL;

    // Initialize the command_flags.
    cmd_data->print_average = false;
    cmd_data->print_each_iter = true;
    cmd_data->dst_align = 0;
    cmd_data->src_align = 0;
    cmd_data->num_args = 0;
    cmd_data->cpu_to_lock = -1;
    cmd_data->data_size = DEFAULT_DATA_SIZE;
    for (int i = 0; i < MAX_ARGS; i++) {
        cmd_data->args[i] = -1;
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            int *save_value = NULL;
            if (strcmp(argv[i], "--print_average") == 0) {
              cmd_data->print_average = true;
            } else if (strcmp(argv[i], "--no_print_each_iter") == 0) {
              cmd_data->print_each_iter = false;
            } else if (strcmp(argv[i], "--dst_align") == 0) {
              save_value = &cmd_data->dst_align;
            } else if (strcmp(argv[i], "--src_align") == 0) {
              save_value = &cmd_data->src_align;
            } else if (strcmp(argv[i], "--lock_to_cpu") == 0) {
              save_value = &cmd_data->cpu_to_lock;
            } else if (strcmp(argv[i], "--data_size") == 0) {
              save_value = &cmd_data->data_size;
            } else {
                printf("Unknown option %s\n", argv[i]);
                return NULL;
            }
            if (save_value) {
                // Checking both characters without a strlen() call should be
                // safe since as long as the argument exists, one character will
                // be present (\0). And if the first character is '-', then
                // there will always be a second character (\0 again).
                if (i == argc - 1 || (argv[i + 1][0] == '-' && !isdigit(argv[i + 1][1]))) {
                    printf("The option %s requires one argument.\n",
                           argv[i]);
                    return NULL;
                }
                *save_value = atoi(argv[++i]);
            }
        } else if (!command) {
            for (function_t *function = function_table; function->name != NULL; function++) {
                if (strcmp(argv[i], function->name) == 0) {
                    command = function;
                    break;
                }
            }
            if (!command) {
                printf("Uknown command %s\n", argv[i]);
                return NULL;
            }
        } else if (cmd_data->num_args > MAX_ARGS) {
            printf("More than %d number arguments passed in.\n", MAX_ARGS);
            return NULL;
        } else {
            cmd_data->args[cmd_data->num_args++] = atoi(argv[i]);
        }
    }

    // Check the arguments passed in make sense.
    if (cmd_data->num_args != 1 && cmd_data->num_args != 2) {
        printf("Not enough arguments passed in.\n");
        return NULL;
    } else if (cmd_data->dst_align < 0) {
        printf("The --dst_align option must be greater than or equal to 0.\n");
        return NULL;
    } else if (cmd_data->src_align < 0) {
        printf("The --src_align option must be greater than or equal to 0.\n");
        return NULL;
    } else if (cmd_data->data_size <= 0) {
        printf("The --data_size option must be a positive number.\n");
        return NULL;
    } else if ((cmd_data->dst_align & (cmd_data->dst_align - 1))) {
        printf("The --dst_align option must be a power of 2.\n");
        return NULL;
    } else if ((cmd_data->src_align & (cmd_data->src_align - 1))) {
        printf("The --src_align option must be a power of 2.\n");
        return NULL;
    }

    return command;
}

bool raisePriorityAndLock(int cpu_to_lock) {
    cpu_set_t cpuset;

    if (setpriority(PRIO_PROCESS, 0, -20)) {
        perror("Unable to raise priority of process.\n");
        return false;
    }

    CPU_ZERO(&cpuset);
    if (sched_getaffinity(0, sizeof(cpuset), &cpuset) != 0) {
        perror("sched_getaffinity failed");
        return false;
    }

    if (cpu_to_lock < 0) {
        // Lock to the last active core we find.
        for (int i = 0; i < CPU_SETSIZE; i++) {
            if (CPU_ISSET(i, &cpuset)) {
                cpu_to_lock = i;
            }
        }
    } else if (!CPU_ISSET(cpu_to_lock, &cpuset)) {
        printf("Cpu %d does not exist.\n", cpu_to_lock);
        return false;
    }

    if (cpu_to_lock < 0) {
        printf("Cannot find any valid cpu to lock.\n");
        return false;
    }

    CPU_ZERO(&cpuset);
    CPU_SET(cpu_to_lock, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
        perror("sched_setaffinity failed");
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    command_data_t cmd_data;

    function_t *command = processOptions(argc, argv, &cmd_data);
    if (!command) {
      usage();
      return -1;
    }

    if (!raisePriorityAndLock(cmd_data.cpu_to_lock)) {
      return -1;
    }

    printf("%s\n", command->name);
    return (*command->ptr)(cmd_data);
}
