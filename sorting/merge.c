#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#define SEED  100
#define UPPER 100
#define LOWER  1

int generate_random_number(unsigned int lower, unsigned int upper);
void initialize_arr();
void create_threads(pthread_t* threads);
void join_threads(pthread_t* threads);
void print_time_elapsed(struct timeval *start, struct timeval *end);

void merge_sort(long data[], int left, int right);
void merge(long data[], int left, int middle, int right);
void* thread_merge_sort(void* arg);
void merge_sections_of_array(long data[], int number, int aggregation);
void print_merged_array(long data[]);
void validate_merged_array(long data[]);

#define NUM_OF_THREADS 2
#define ARR_LENGTH 1000000

// critical section
long data[ARR_LENGTH];

const int NUMBERS_PER_THREAD = ARR_LENGTH / NUM_OF_THREADS;
const int OFFSET = ARR_LENGTH % NUM_OF_THREADS;

int main(int argc, const char* argv[])
{
    initialize_arr();
    srand(SEED);

    struct timeval start, end;
    pthread_t threads[NUM_OF_THREADS];

    gettimeofday(&start, NULL);
    create_threads(threads);
    join_threads(threads);
    merge_sections_of_array(data, NUM_OF_THREADS, 1);
    gettimeofday(&end, NULL);


    print_time_elapsed(&start, &end);
    //print_merged_array(data);
    validate_merged_array(data);

    return 0;
}

void initialize_arr()
{
    for (long i = 0; i < ARR_LENGTH; i++)
    {
        data[i] = generate_random_number(UPPER, LOWER);
    }
}

void create_threads(pthread_t threads[])
{
    for (long i = 0; i < NUM_OF_THREADS; i++)
    {
        int rc = pthread_create(&threads[i], NULL, thread_merge_sort, (void *) i);
        if (rc)
        {
            printf("ERROR: pthread_create(): %d\n", rc);
            exit(-1);
        }
    }
}

void join_threads(pthread_t* threads)
{
    for(int i = 0; i < NUM_OF_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }
}

void print_time_elapsed(struct timeval *start, struct timeval *end)
{
    double usec = (double) (end->tv_usec - start->tv_usec) / 1000000;
    double sec = (double) (end->tv_sec - start->tv_sec);
    double time_spent = (double) (usec + sec);

    printf("Time elapsed: %fs\n", time_spent);
}

int generate_random_number(unsigned int upper, unsigned int lower)
{
    return ((double) rand() / RAND_MAX) * (upper - lower) + lower;
}

void merge_sections_of_array(long data[], int num_of_threads, int aggregation)
{
    for (int i = 0; i < num_of_threads; i = i + 2)
    {
        int left = i * (NUMBERS_PER_THREAD * aggregation);
        int middle = left + (NUMBERS_PER_THREAD * aggregation) - 1;
        int right = ((i + 2) * NUMBERS_PER_THREAD * aggregation) - 1;

        if (right >= ARR_LENGTH)
        {
            right = ARR_LENGTH - 1;
        }
        merge(data, left, middle, right);
    }

    if (num_of_threads / 2 >= 1)
    {
        merge_sections_of_array(data, num_of_threads / 2, aggregation * 2);
    }
}

void* thread_merge_sort(void* arg)
{
    int thread_id = (long) arg;
    int left = thread_id * NUMBERS_PER_THREAD;
    int right = (thread_id + 1) * NUMBERS_PER_THREAD - 1;

    if (thread_id == NUM_OF_THREADS - 1)
    {
        right += OFFSET;
    }

    int middle = left + (right - left) / 2;

    if (left < right)
    {
        merge_sort(data, left, right);
        merge_sort(data, left + 1, right);
        merge(data, left, middle, right);
    }
    return NULL;
}

void print_merged_array(long data[])
{
    printf("Sorted: ");
    for (long i = 0; i < ARR_LENGTH; i++)
    {
        printf("%ld ", data[i]);
    }
    printf("\n");
}

void validate_merged_array(long data[]) {
    long max = 0;
    for (int i = 1; i < ARR_LENGTH; i ++)
    {
        if (data[i] >= data[i - 1])
        {
            max = data[i];
        } else
        {
            printf("Error. Out of order sequence: %ld found\n", data[i]);
            return;
        }
    }
    printf("Array is in sorted order\n");
}

void merge_sort(long data[], int left, int right)
{
    if (left < right)
    {
        int middle = left + (right - left) / 2;
        merge_sort(data, left, middle);
        merge_sort(data, middle + 1, right);
        merge(data, left, middle, right);
    }
}

void merge(long data[], int left, int middle, int right)
{
    int left_arr_length = middle - left + 1;
    int right_arr_length = right - middle;
    long left_array[left_arr_length];
    long right_array[right_arr_length];

    /* copy values to left array */
    for (int i = 0; i < left_arr_length; i++)
    {
        left_array[i] = data[left + i];
    }

    /* copy values to right array */
    for (int j = 0; j < right_arr_length; j ++)
    {
        right_array[j] = data[middle + 1 + j];
    }

    int i = 0, j = 0, k = 0;
    /** chose from right and left arrays and copy */
    while (i < left_arr_length && j < right_arr_length)
    {
        if (left_array[i] <= right_array[j])
        {
            data[left + k] = left_array[i];
            i ++;
        } else
        {
            data[left + k] = right_array[j];
            j ++;
        }
        k ++;
    }

    /* copy the remaining values to the array */
    while (i < left_arr_length)
    {
        data[left + k] = left_array[i];
        k++;
        i++;
    }
    while (j < right_arr_length)
    {
        data[left + k] = right_array[j];
        k++;
        j++;
    }
}