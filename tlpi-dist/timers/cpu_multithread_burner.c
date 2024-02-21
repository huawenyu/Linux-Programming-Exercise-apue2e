/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2023.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Supplementary program for Chapter 23 */

/* cpu_multithread_burner.c

   Usage: cpu_multithread_burner period...

   This program creates one thread per command-line argument. Each thread
   loops consuming CPU, and, after each 'period' seconds of elapsed time,
   reports its thread ID and rate of CPU consumption since the last report.

   For some experiments, it is useful to confine all threads to the
   same CPU, using taskset(1). For example:

        taskset 0x1 ./cpu_multithread_burner 2 2

   See also cpu_multi_burner.c.
*/
#define _GNU_SOURCE
#include <sys/syscall.h>
#include <sched.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include "tlpi_hdr.h"

#if !defined(__GLIBC__) || __GLIBC_MINOR__ < 30

/* glibc 2.30 has a gettid() wrapper */

static pid_t
gettid(void)
{
    return syscall(SYS_gettid);
}
#endif

#define NANO 1000000000L

static long
timespecDiff(struct timespec a, struct timespec b)
{
    return (b.tv_sec - a.tv_sec) * NANO + b.tv_nsec - a.tv_nsec;
}

static void *
threadFunc(void *arg)
{
    float period;       /* Interval (in CPU seconds) for displaying statistics
                           on consumed CPU time */
    char *sarg = arg;
    sscanf(sarg, "%f", &period);

    long step_size = NANO * period;
    long prev_step = 0;

    struct timespec base_real;
    if (clock_gettime(CLOCK_REALTIME, &base_real) == -1)
        errExit("clock_gettime");

    struct timespec prev_real = base_real;

    struct timespec prev_cpu;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &prev_cpu) == -1)
        errExit("clock_gettime");

    while (1) {
        struct timespec curr_real;
        if (clock_gettime(CLOCK_REALTIME, &curr_real) == -1)
            errExit("clock_gettime");

        long elapsed_real_nsec = timespecDiff(base_real, curr_real);
        long elapsed_real_steps = elapsed_real_nsec / step_size;

        if (elapsed_real_steps > prev_step) {
            struct timespec curr_cpu;
            if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &curr_cpu) == -1)
                errExit("clock_gettime");

            long diff_real_nsec = timespecDiff(prev_real, curr_real);
            long diff_cpu_nsec = timespecDiff(prev_cpu, curr_cpu);

            printf("%ld  [t=%.2f (delta: %.2f)]   %%CPU = %5.2f\n",
                    (long) gettid(),
                    (double) elapsed_real_nsec / NANO,
                    (double) diff_real_nsec / NANO,
                    (double) diff_cpu_nsec / diff_real_nsec * 100);

            prev_cpu = curr_cpu;
            prev_real = curr_real;
            prev_step = elapsed_real_steps;
        }
    }

    return NULL;
}

int
main(int argc, char *argv[])
{
    pthread_t thr;
    int j, s;

    if (argc < 2 || strcmp(argv[1], "--help") == 0)
        usageErr("%s [period]...\n"
                "Creates one thread per argument that reports "
                "CPU usage each 'period' seconds.\n"
                "'period' can be a floating-point number\n", argv[0]);

    /* Create one thread per command-line argument */

    for (j = 1; j < argc; j++) {
        s = pthread_create(&thr, NULL, threadFunc, argv[j]);
        if (s != 0)
            errExitEN(s, "pthread_create");
    }

    pause();

    exit(EXIT_SUCCESS);
}
