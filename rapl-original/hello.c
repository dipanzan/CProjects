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

#include "rapl-read.h"

#define NUM_RAPL_DOMAINS 5
#define MAX_PACKAGES 5
#define EVENT_UNITS 3

static int package_map[MAX_PACKAGES];

static int total_cores = 0,total_packages = 0;

char *amd_rapl_domain = "energy-pkg";

static int perf_event_open(struct perf_event_attr *hw_event_uptr,
                    pid_t pid, int cpu, int group_fd, unsigned long flags) {

        return syscall(__NR_perf_event_open,hw_event_uptr, pid, cpu,
                        group_fd, flags);
}

int check_paranoid()
{
	return 0;
}

int main(void)
{
	detect_power_infrastructure();

	return 0;
}

int get_power_type()
{
	int type;

	FILE *file = fopen("/sys/bus/event_source/devices/power/type", "r");
	if (file == NULL) {
		printf("perf_event not supported!");
	}
	fscanf(file,"%d", &type);
	fclose(file);

	return type;
}

int read_config()
{
	FILE *file;
	char filename[BUFSIZ];
	sprintf(filename, "/sys/bus/event_source/devices/power/events/%s", amd_rapl_domain);

	int config = 0;
	if (file != NULL) 
	{
		fscanf(file, "event = %x", &config);
		fclose(file);
	}
	return config;
}

// void detect_power_infrastructure()
// {
// 	FILE *file[EVENT_UNITS];
	

// 	int config = 0;
// 	char units[NUM_RAPL_DOMAINS][BUFSIZ];
// 	double scale[NUM_RAPL_DOMAINS];
// 	char power_infrastructure[BUFSIZ];

	
// 	sprintf(filename[1],"/sys/bus/event_source/devices/power/events/%s.scale", amd_rapl_domain);
// 	sprintf(filename[2],"/sys/bus/event_source/devices/power/events/%s.unit", amd_rapl_domain);

// 	for (int i = 0; i < EVENT_UNITS; i++)
// 	{
// 		file[i] = fopen(filename[i], "r");
// 		if (file[i] == NULL)
// 		{
// 			goto abort;
// 		}
// 	}
	
	
// 	sprintf(power_infrastructure, "Event: %s, Config: %d", amd_rapl_domain, config);


// 	abort:
// 		return;


	
// 	file=fopen(filename,"r");

// 	if (file!=NULL) {
// 		fscanf(file,"%lf",&scale[i]);
// 		printf("scale=%g ",scale[i]);
// 		fclose(file);
// 	}

	// 
	// file=fopen(filename,"r");

	// if (file!=NULL) {
	// 	fscanf(file,"%s", units[i]);
	// 	printf("units=%s ", units[i]);
	// 	fclose(file);
	// }

	// printf("\n");

//}

// static int rapl_perf(int core)
// {
// 	int fd[NUM_RAPL_DOMAINS][MAX_PACKAGES];
	
// 	struct perf_event_attr attr;
// 	long long value;
// 	int i,j;
// 	int paranoid_value;

// 	int type = get_power_type();

// 	for(j=0;j<total_packages;j++) {

// 		for(i=0;i<NUM_RAPL_DOMAINS;i++) {

// 			fd[i][j]=-1;

// 			memset(&attr,0x0,sizeof(attr));
// 			attr.type=type;
// 			attr.config=config[i];
// 			if (config[i]==0) continue;

// 			fd[i][j]=perf_event_open(&attr,-1, package_map[j],-1,0);
// 			if (fd[i][j]<0) {
// 				if (errno==EACCES) {
// 					paranoid_value=check_paranoid();
// 					if (paranoid_value>0) {
// 						printf("\t/proc/sys/kernel/perf_event_paranoid is %d\n",paranoid_value);
// 						printf("\tThe value must be 0 or lower to read system-wide RAPL values\n");
// 					}

// 					printf("\tPermission denied; run as root or adjust paranoid value\n\n");
// 					return -1;
// 				}
// 				else {
// 					printf("\terror opening core %d config %d: %s\n\n",
// 						package_map[j], config[i], strerror(errno));
// 					return -1;
// 				}
// 			}
// 		}
// 	}

// 	printf("\n\tSleeping 1 second\n\n");
// 	sleep(1);

// 	for(j=0;j<total_packages;j++) {
// 		printf("\tPackage %d:\n",j);

// 		for(i=0;i<NUM_RAPL_DOMAINS;i++) {

// 			if (fd[i][j]!=-1) {
// 				read(fd[i][j],&value,8);
// 				close(fd[i][j]);

// 				printf("\t\t%s Energy Consumed: %lf %s\n",
// 					rapl_domain_names[i],
// 					(double)value*scale[i],
// 					units[i]);

// 			}

// 		}

// 	}
// 	printf("\n");

// 	return 0;
// }