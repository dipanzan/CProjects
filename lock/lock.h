#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>

static void send_cpu_to_kernel_module(int cpu);
static void set_max_speed_for_cpu();
static void reset_all_cpus();
static void set_pid_to_env();
static unsigned long get_random_number(unsigned long min, unsigned long max);
static void *critical_section(void *data);
static int get_running_cpu();
static void allocate_primes();
static void deallocate_primes();
static void stress_primes(unsigned long n);
static void print_primes();

typedef struct _pr
{
    unsigned long r1;
    unsigned long r2;
} pr;