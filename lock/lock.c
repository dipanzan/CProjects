#include "lock.h"

#define WR_VALUE _IOW('a', 'a', int *)
#define RD_VALUE _IOR('a', 'b', int *)

static void send_cpu_to_kernel_module(int cpu)
{
    int fd;

    fd = open("/dev/etx_device", O_RDWR);
    if (fd < 0)
    {
        printf("cannot open device file\n");
        exit(-1);
    }
    ioctl(fd, WR_VALUE, (int32_t *)&cpu);

    // printf("Reading Value from Driver\n");
    // ioctl(fd, RD_VALUE, (int32_t*) &value);
    // printf("Value is %d\n", value);

    printf("closing driver\n");
    close(fd);
}

pthread_mutex_t rs_mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned long *primes;

static void reset_all_cpus()
{
    printf("resetting all CPUs\n");
    char *cmd = "./reset-governor.sh";
    char *argv[] = {cmd, NULL};
    int result = execvp(cmd, argv);

    if (result == -1)
    {
        printf("FAIL: couldn't reset all CPUs\n");
        printf("exec failed: %s\n", strerror(errno));
        exit(1);
    }
}

static void set_max_speed_for_cpu(int cpu)
{
    printf("setting CPU [%d] to max speed\n", cpu);
    char *cmd = "./governor.sh";
    char cpu_str[3];
    sprintf(cpu_str, "%d", cpu);
    char *argv[] = {cmd, cpu_str, NULL};

    int result = execvp(cmd, argv);
    if (result == -1)
    {
        printf("FAIL: couldn't set cpu [%d] max speed\n", cpu);
        printf("exec failed: %s\n", strerror(errno));
        exit(1);
    }
}

static int get_running_cpu()
{
    int cpu = sched_getcpu();

    if (cpu == -1)
    {
        printf("FAIL: couldn't retrive currently running CPU");
        exit(1);
    }
    printf("SUCCESS: current CPU: %d\n", cpu);
    return cpu;
}

static void allocate_primes(unsigned long n)
{
    primes = malloc(sizeof(unsigned long) * n);
    if (!primes)
    {
        printf("FAIL: Not enough memory for %ld primes\n", n);
        exit(1);
    }
}

static void deallocate_primes()
{
    free(primes);
}

static void stress_primes(unsigned long n)
{
    for (unsigned long i = 0, p = 2; i < n; i++, p++)
    {
        if (p % 2 == 0)
        {
            primes[i] = p;
        }
    }
}

static void print_primes(unsigned long n)
{
    for (int i = 0; i < n; i++)
    {
        printf("%ld, ", primes[i]);
    }
    printf("\n");
}

static unsigned long get_random_number(unsigned long max, unsigned long min)
{
    return rand() % (max + 1 - min) + min;
}

static void *critical_section(void *data)
{
    unsigned long r1 = ((pr *)data)->r1;
    unsigned long r2 = ((pr *)data)->r2;

    pid_t tid = syscall(__NR_gettid);
    printf("[%d] primes: %ld\n", tid, r1);

    int cpu = get_running_cpu();
    // send_cpu_to_kernel_module(cpu);
    // set_max_speed_for_cpu(cpu);

    pthread_mutex_lock(&rs_mutex);
    allocate_primes(r1);
    stress_primes(r1);
    // print_primes(r1);
    deallocate_primes();
    pthread_mutex_unlock(&rs_mutex);

    // reset_all_cpus();
}

static void set_pid_to_env()
{
    char pid_str[6];
    int pid = getpid();

    sprintf(pid_str, "%d", pid);
    printf("PID: %s\n", pid_str);

    int setenv_rc = setenv("LOCK_C_PID", pid_str, 1);
    printf("setenv_rc: %d\n", setenv_rc);
}

int main(int argc, char *argv[])
{
    // set_pid_to_env();
    // pid_t tid = syscall(__NR_gettid);
    // printf("[%d] main() sleeping for 1s\n", tid);

    int pid = getpid();
    printf("PID: %d\n", pid);
    srand(time(NULL));

    unsigned long r1 = get_random_number(1000000000, 1000000000);
    unsigned long r2 = get_random_number(1000000000, 1000000000);

    pr ranges = {r1, r2};

    pthread_t thread1, thread2;
    int rc1, rc2;

    pthread_mutex_init(&rs_mutex, NULL);

    rc1 = pthread_create(&thread1, NULL, &critical_section, (void *)&ranges);
    rc2 = pthread_create(&thread2, NULL, &critical_section, (void *)&ranges);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    pthread_mutex_destroy(&rs_mutex);

    return 0;
}
