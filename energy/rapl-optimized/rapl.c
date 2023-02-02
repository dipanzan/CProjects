/* Read the RAPL registers on recent (>sandybridge) Intel processors

There are currently three ways to do this:
    1. Read the MSRs directly with /dev/cpu/??/msr
    2. Use the perf_event_open() interface
    3. Read the values from the sysfs powercap interface

MSR Code originally based on a (never made it upstream) linux-kernel
    RAPL driver by Zhang Rui <rui.zhang@intel.com>
    https://lkml.org/lkml/2011/5/26/93

For raw MSR access the /dev/cpu/??/msr driver must be enabled and
    permissions set to allow read access.
    You might need to "modprobe msr" before it will work.

perf_event_open() support requires at least Linux 3.14 and to have
    /proc/sys/kernel/perf_event_paranoid < 1

the sysfs powercap interface got into the kernel in
    2d281d8196e38dd (3.13) */

// Compile with:   gcc -O2 -Wall -o rapl-read rapl-read.c -lm

// cat /sys/devices/system/cpu/cpu*/topology/physical_package_id
// cat /proc/sys/kernel/perf_event_paranoid
// cat /sys/bus/event_source/devices/power/type
// cat /sys/bus/event_source/devices/power/events/energy-pkg
// cat /sys/bus/event_source/devices/power/events/energy-pkg.scale
// cat /sys/bus/event_source/devices/power/events/energy-pkg.unit

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <sys/syscall.h>
#include <linux/perf_event.h>

#define MAX_CPUS 1024
#define MAX_PACKAGES 16
#define PID_NEGATIVE_ONE -1

#define NUM_RAPL_DOMAINS 1
#define TOTAL_PACKAGES 1
#define TOTAL_CORES 16
#define TYPE 12
#define CONFIG 0x02
#define SCALE 2.3283064365386962890625e-10
#define UNIT "Joule"

static void open_fd(int *fd, int pid, int core);
static void close_fd(int *fd, int core);
static void rapl_perf(pid_t pid, int core);
static int perf_event_open(struct perf_event_attr *hw_event_uptr, pid_t pid, int cpu, int group_fd, unsigned long flags);

static int perf_event_open(struct perf_event_attr *hw_event_uptr, pid_t pid, int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event_uptr, pid, cpu, group_fd, flags);
}

static void open_fd(int *fd, int pid, int core)
{
    struct perf_event_attr attr;

    memset(&attr, 0x0, sizeof(attr));
    attr.type = TYPE;
    attr.config = CONFIG;

    *fd = perf_event_open(&attr, pid, core, -1, 0);
}

static void close_fd(int *fd, int core)
{
    if (*fd != -1)
    {
        unsigned long long value;
        read(*fd, &value, 8);
        close(*fd);
        printf("[CPU: %d] energy consumed: %lf %s\n", core, (double)value * SCALE, UNIT);
    }
}

static void rapl_perf(pid_t pid, int core)
{
    int fd = -1;

    open_fd(&fd, pid, core);
    sleep(1);
    close_fd(&fd, core);
}

int main(int argc, char *argv[])
{
    // int core = atoi(argv[1]);
    rapl_perf(PID_NEGATIVE_ONE, 0);

    return 0;
}
