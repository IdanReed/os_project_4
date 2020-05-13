#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define DEFAULT_THREAD_COUNT 12

#define MAX_BATCH_SIZE 1000000


int thread_count;

char* lines[MAX_BATCH_SIZE] = { NULL };
size_t line_sizes[MAX_BATCH_SIZE];
ssize_t line_lens[MAX_BATCH_SIZE];
long sums[MAX_BATCH_SIZE];

pthread_mutex_t mutexsum;			// mutex for char_counts

int current_batch_size;

long sum_line(int idx)
{
  long sum = 0;
  int i;

  for(i = 0; i < line_lens[idx]; i++){
    sum += lines[idx][i];
  }

  return sum;
}

void * run_thread(void * my_id)
{
  int i;
  int start_pos = ((int) my_id) * (current_batch_size / thread_count);
  int end_pos = start_pos + (current_batch_size / thread_count);
  const int count = end_pos - start_pos;
  long *local_sums;

  #pragma omp private(myID,i,start_pos,end_pos,count,local_sums)
	{
    start_pos = ((int) my_id) * (current_batch_size / thread_count);
    end_pos = start_pos + (current_batch_size / thread_count);
    count = end_pos - start_pos;
    local_sums = malloc(sizeof(*local_sums) * count );

    for(i = start_pos; i < end_pos; i++){
      local_sums[i - start_pos] = sum_line(i);
    }

    //pthread_mutex_lock (&mutexsum);
    for(i = 0; i < count; i++){
      sums[i + start_pos] = local_sums[i];
    }
    //pthread_mutex_unlock (&mutexsum);

    //pthread_exit(NULL);
    free(local_sums);
  }
}

void print_batch(int batch_cnt)
{
  int i;
  for(i = 1; i < current_batch_size; i++)
  {
    int diff = sums[i-1] - sums[i];
    printf("%d - %d: %d\n", i-1 + (batch_cnt * MAX_BATCH_SIZE), i + (batch_cnt * MAX_BATCH_SIZE), diff);
  }
}

int read_lines(FILE * fp){
  int line_cnt = 0;
  while ((line_lens[line_cnt] = getline(&lines[line_cnt], &line_sizes[line_cnt], fp)) != -1) {
    line_cnt++;

    if(line_cnt >= MAX_BATCH_SIZE)
      break;
  }
  return line_cnt;
}

int main(int argc, char * argv[])
{

  pthread_mutex_init(&mutexsum, NULL);
  thread_count = DEFAULT_THREAD_COUNT;
  char * perf_out = "performance.csv";

  if( argc > 2) {
    thread_count = atoi(argv[1]);
    perf_out = argv[2];
  }


  int i, rc;
  int line_cnt = MAX_BATCH_SIZE;
  int batch_cnt = 0;
  pthread_t threads[thread_count];
  pthread_attr_t attr;
  void * status;

  FILE * fp;
  FILE * out;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  double summation_time = 0;

  struct timeval start, end;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  fp = fopen("/homes/dan/625/wiki_dump.txt", "r");
  if (fp == NULL)
      exit(EXIT_FAILURE);

  while(line_cnt >= MAX_BATCH_SIZE){
    line_cnt = read_lines(fp);

    current_batch_size = line_cnt;

    gettimeofday(&start, NULL);
    // Start Threads
    for(i = 0; i < thread_count; i++){
      // rc = pthread_create(&threads[i], &attr, run_thread, (void *) i);
      // if (rc) {
      //   printf("ERROR: return code from pthread_create() is %d\n", rc);
      //   exit(EXIT_FAILURE);
      // }
      #pragma omp parallel
      {
        run_thread(void * i);
      }
    }


    // Join threads
    // pthread_attr_destroy(&attr);
    // for(i = 0; i < thread_count; i++){
    //   rc = pthread_join(threads[i], &status);
    //   if (rc) {
    //     printf("ERROR: return code from pthread_join() is %d\n", rc);
    //     exit(EXIT_FAILURE);
    //   }
    // }
    // gettimeofday(&end, NULL);

    summation_time += end.tv_sec + end.tv_usec / 1e6 -
                      start.tv_sec - start.tv_usec / 1e6;


    print_batch(batch_cnt);


    batch_cnt++;
  }

  fclose(fp);

  for(i = 0; i < MAX_BATCH_SIZE; i++){
    free(lines[i]);
  }

  out = fopen(perf_out, "a");
  fprintf(out, "%d, %f\n", thread_count, summation_time);

  fclose(out);

	// pthread_mutex_destroy(&mutexsum);
	// pthread_exit(NULL);
}