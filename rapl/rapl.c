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

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

#define MAX_CPUS 1024
#define MAX_PACKAGES 16
#define PID_NEGATIVE_ONE -1

static int total_cores = 0, total_packages = 0;
static int package_map[MAX_PACKAGES];

static void open_fd(int fd[][MAX_PACKAGES], int pid, int core);
static void close_fd(int fd[][MAX_PACKAGES], int core);
static void measure_cores(int core);
static void measure_energy_consumption(int argc, char *argv[]);
static void detect_cpu();
static void detect_packages();
static void reset_package_map();
static void sleep_experiment(int time);
static void launch_experiment();
static void rapl_perf(pid_t pid, int core);
static int get_perf_event_rapl_type();
static int get_perf_event_rapl_config();
static double get_perf_event_rapl_scale();
static void get_perf_event_rapl_units(char *);

static void measure_cores(int core)
{
	if (core != -1)
	{
		rapl_perf(PID_NEGATIVE_ONE, core);
	}
	else
	{
		for (int cpu = 0; cpu < total_cores; cpu++)
		{
			rapl_perf(PID_NEGATIVE_ONE, cpu);
		}
	}
}

static void sleep_experiment(int time)
{

	printf("sleeping for %d\n\n", time);
	sleep(time);
}

static void launch_experiment()
{
	printf("launching experiment:\n\n");
	int result = execvp("./lock", NULL);

	if (result == -1)
	{
		printf("couldn't launch experiement\n");
	}
}

static int open_msr(int core)
{

	char msr_filename[BUFSIZ];
	int fd;

	sprintf(msr_filename, "/dev/cpu/%d/msr", core);
	fd = open(msr_filename, O_RDONLY);
	if (fd < 0)
	{
		if (errno == ENXIO)
		{
			fprintf(stderr, "rdmsr: No CPU %d\n", core);
			exit(2);
		}
		else if (errno == EIO)
		{
			fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n",
					core);
			exit(3);
		}
		else
		{
			perror("rdmsr:open");
			fprintf(stderr, "Trying to open %s\n", msr_filename);
			exit(127);
		}
	}

	return fd;
}

static long long read_msr(int fd, int which)
{
	uint64_t data;

	if (pread(fd, &data, sizeof data, which) != sizeof data)
	{
		perror("rdmsr:pread");
		exit(127);
	}
	return (long long)data;
}

static void detect_cpu()
{
	FILE *f = fopen("/proc/cpuinfo", "r");

	if (!f)
	{
		printf("could not open /proc/cpuinfo\n");
		exit(1);
	}

	char buffer[BUFSIZ], vendor_id[BUFSIZ], *result;
	while (true)
	{
		result = fgets(buffer, BUFSIZ, f);
		if (!result)
		{
			break;
		}

		if (!strncmp(result, "vendor_id", 8))
		{
			sscanf(result, "%*s%*s%s", vendor_id);
			printf("CPU: %s\n\n", vendor_id);
			break;
		}
	}
	fclose(f);
}

static void reset_package_map()
{
	for (int i = 0; i < MAX_PACKAGES; i++)
	{
		package_map[i] = -1;
	}
}
static void detect_packages()
{
	reset_package_map();

	char filename[BUFSIZ];
	FILE *f;
	int i, package;

	for (i = 0; i < MAX_CPUS; i++)
	{
		sprintf(filename, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", i);
		f = fopen(filename, "r");
		if (!f)
		{
			break;
		}

		fscanf(f, "%d", &package);
		printf("%d (%d)", i, package);

		i % 8 == 7 ? printf("\n") : printf(", ");

		fclose(f);

		if (package_map[package] == -1)
		{
			total_packages++;
			package_map[package] = i;
		}
	}

	total_cores = i;
	printf("Detected %d cores in %d packages\n\n", total_cores, total_packages);
}

static int perf_event_open(struct perf_event_attr *hw_event_uptr, pid_t pid, int cpu, int group_fd, unsigned long flags)
{
	return syscall(__NR_perf_event_open, hw_event_uptr, pid, cpu, group_fd, flags);
}

#define NUM_RAPL_DOMAINS 1

char rapl_domain_names[NUM_RAPL_DOMAINS][30] = {
	"energy-pkg",
};

static int check_paranoid(void)
{
	int paranoid_value;
	FILE *f = fopen("/proc/sys/kernel/perf_event_paranoid", "r");
	if (!f)
	{
		fprintf(stderr, "Error! could not open /proc/sys/kernel/perf_event_paranoid %s\n", strerror(errno));
		// return non-negative paranoid value, since negative means no paranoia
		return 404;
	}
	fscanf(f, "%d", &paranoid_value);
	fclose(f);

	return paranoid_value;
}

static int get_perf_event_rapl_type()
{
	FILE *f = fopen("/sys/bus/event_source/devices/power/type", "r");
	if (!f)
	{
		printf("could not retrieve perf_event_rapl type\n");
		exit(-1);
	}
	int type = 0;
	fscanf(f, "%d", &type);
	fclose(f);

	return type;
}

static int get_perf_event_rapl_config()
{
	char *filename = "/sys/bus/event_source/devices/power/events/energy-pkg";
	FILE *f = fopen(filename, "r");
	if (!f)
	{
		printf("could not retrieve perf_event_rapl config\n");
		exit(-1);
	}
	int config = 0;
	fscanf(f, "event = %x", &config);
	fclose(f);

	return config;
}

static double get_perf_event_rapl_scale()
{
	char *filename = "/sys/bus/event_source/devices/power/events/energy-pkg.scale";
	FILE *f = fopen(filename, "r");
	if (!f)
	{
		printf("could not retrieve perf_event_rapl scale\n");
		exit(-1);
	}
	double scale = 0.0;
	fscanf(f, "%lf", &scale);
	fclose(f);

	return scale;
}

static void get_perf_event_rapl_units(char *units)
{
	char *filename = "/sys/bus/event_source/devices/power/events/energy-pkg.unit";
	FILE *f = fopen(filename, "r");

	if (!f)
	{
		printf("could not retrieve perf_event rapl units\n");
		exit(-1);
	}
	fscanf(f, "%s", units);
	fclose(f);
}

static void open_fd(int fd[][MAX_PACKAGES], int pid, int core)
{
	int type = get_perf_event_rapl_type();
	int config = get_perf_event_rapl_config();
	int paranoid = check_paranoid();
	struct perf_event_attr attr;

	for (int j = 0; j < total_packages; j++)
	{
		for (int i = 0; i < NUM_RAPL_DOMAINS; i++)
		{
			fd[i][j] = -1;
			memset(&attr, 0x0, sizeof(attr));
			attr.type = type;
			attr.config = config;
			// int perf_event_open(struct perf_event_attr *attr,
			//                     pid_t pid, int cpu, int group_fd,
			//                     unsigned long flags);
			// fd[i][j] = perf_event_open(&attr, -1, package_map[j], -1, 0);

			fd[i][j] = perf_event_open(&attr, pid, core, -1, 0);

			if (fd[i][j] < 0)
			{
				if (errno == EACCES)
				{
					printf("\n\tNeed root privileges or perf_event_paranoid < 0\n\n");
					paranoid = check_paranoid();

					if (paranoid > 0)
					{
						printf("\t/proc/sys/kernel/perf_event_paranoid is %d\n", paranoid);
					}
					exit(-1);
				}
				else
				{
					printf("\terror opening core %d config %d: %s\n\n", package_map[j], config, strerror(errno));
					exit(-1);
				}
			}
		}
	}
}

static void close_fd(int fd[][MAX_PACKAGES], int core)
{
	double scale = get_perf_event_rapl_scale();

	char units[BUFSIZ];
	get_perf_event_rapl_units(units);

	long long value;
	for (int j = 0; j < total_packages; j++)
	{
		for (int i = 0; i < NUM_RAPL_DOMAINS; i++)
		{
			if (fd[i][j] != -1)
			{
				read(fd[i][j], &value, 8);
				close(fd[i][j]);
				printf("[CPU: %d] energy consumed: %lf %s\n", core, (double)value * scale, units);
			}
		}
	}
}

static void rapl_perf(pid_t pid, int core)
{
	int fd[NUM_RAPL_DOMAINS][MAX_PACKAGES];

	// measure energy consumption
	open_fd(fd, pid, core);
	sleep_experiment(1);
	close_fd(fd, core);
}

static int rapl_sysfs(int core)
{
	char event_names[MAX_PACKAGES][NUM_RAPL_DOMAINS][256];
	char filenames[MAX_PACKAGES][NUM_RAPL_DOMAINS][256];
	char basename[MAX_PACKAGES][256];
	char tempfile[256];
	long long before[MAX_PACKAGES][NUM_RAPL_DOMAINS];
	long long after[MAX_PACKAGES][NUM_RAPL_DOMAINS];
	int valid[MAX_PACKAGES][NUM_RAPL_DOMAINS];
	int i, j;
	FILE *fff;

	printf("\nTrying sysfs powercap interface to gather results\n\n");

	/* /sys/class/powercap/intel-rapl/intel-rapl:0/ */
	/* name has name */
	/* energy_uj has energy */
	/* subdirectories intel-rapl:0:0 intel-rapl:0:1 intel-rapl:0:2 */

	for (j = 0; j < total_packages; j++)
	{
		i = 0;
		sprintf(basename[j], "/sys/class/powercap/intel-rapl/intel-rapl:%d",
				j);
		sprintf(tempfile, "%s/name", basename[j]);
		fff = fopen(tempfile, "r");
		if (fff == NULL)
		{
			fprintf(stderr, "\tCould not open %s\n", tempfile);
			return -1;
		}
		fscanf(fff, "%s", event_names[j][i]);
		valid[j][i] = 1;
		fclose(fff);
		sprintf(filenames[j][i], "%s/energy_uj", basename[j]);

		/* Handle subdomains */
		for (i = 1; i < NUM_RAPL_DOMAINS; i++)
		{
			sprintf(tempfile, "%s/intel-rapl:%d:%d/name",
					basename[j], j, i - 1);
			fff = fopen(tempfile, "r");
			if (fff == NULL)
			{
				// fprintf(stderr,"\tCould not open %s\n",tempfile);
				valid[j][i] = 0;
				continue;
			}
			valid[j][i] = 1;
			fscanf(fff, "%s", event_names[j][i]);
			fclose(fff);
			sprintf(filenames[j][i], "%s/intel-rapl:%d:%d/energy_uj",
					basename[j], j, i - 1);
		}
	}

	/* Gather before values */
	for (j = 0; j < total_packages; j++)
	{
		for (i = 0; i < NUM_RAPL_DOMAINS; i++)
		{
			if (valid[j][i])
			{
				fff = fopen(filenames[j][i], "r");
				if (fff == NULL)
				{
					fprintf(stderr, "\tError opening %s!\n", filenames[j][i]);
				}
				else
				{
					fscanf(fff, "%lld", &before[j][i]);
					fclose(fff);
				}
			}
		}
	}

	printf("\tSleeping 1 second\n\n");
	sleep(1);

	/* Gather after values */
	for (j = 0; j < total_packages; j++)
	{
		for (i = 0; i < NUM_RAPL_DOMAINS; i++)
		{
			if (valid[j][i])
			{
				fff = fopen(filenames[j][i], "r");
				if (fff == NULL)
				{
					fprintf(stderr, "\tError opening %s!\n", filenames[j][i]);
				}
				else
				{
					fscanf(fff, "%lld", &after[j][i]);
					fclose(fff);
				}
			}
		}
	}

	for (j = 0; j < total_packages; j++)
	{
		printf("\tPackage %d\n", j);
		for (i = 0; i < NUM_RAPL_DOMAINS; i++)
		{
			if (valid[j][i])
			{
				printf("\t\t%s\t: %lfJ\n", event_names[j][i],
					   ((double)after[j][i] - (double)before[j][i]) / 1000000.0);
			}
		}
	}
	printf("\n");

	return 0;
}

int main(int argc, char **argv)
{
	detect_cpu();
	detect_packages();
	measure_energy_consumption(argc, argv);

	return 0;
}

void measure_energy_consumption(int argc, char *argv[])
{
	int c;
	int core = atoi(argv[2]);

	while ((c = getopt(argc, argv, "c:hmps")) != -1)
	{
		switch (c)
		{
		case 'h':
			printf("Usage: %s [-h] [-s|p] n [core]\n\n", argv[0]);
			exit(0);
		case 'p':
			measure_cores(core);
			exit(0);
		case 's':
			rapl_sysfs(0);
			exit(0);
		default:
			fprintf(stderr, "Unknown option %c\n", c);
			exit(-1);
		}
	}
}