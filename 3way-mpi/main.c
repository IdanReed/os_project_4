#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define DEFAULT_THREAD_COUNT 12

#define MAX_BATCH_SIZE 120000
#define MAX_LINE_LENGTH 3000

int thread_count;

char lines[MAX_BATCH_SIZE][MAX_LINE_LENGTH] = {{'0'}};
size_t line_sizes[MAX_BATCH_SIZE];
ssize_t line_lens[MAX_BATCH_SIZE];
long *sums;

int block_size;

long sum_line(int idx)
{
  long sum = 0;
  int i;

  for(i = 0; i < line_lens[idx]; i++){
    sum += lines[idx][i];
  }

  return sum;
}

void * run_thread(int my_id)
{
  int i;
  int start_pos;
  int end_pos;
  int count;
  long *local_sums;

  start_pos = ((int) my_id) * block_size;
  end_pos = start_pos + block_size;
  local_sums = malloc(sizeof(long) * block_size);

  for(i = start_pos; i < end_pos && i < MAX_BATCH_SIZE; i++)
  {
    local_sums[i] = sum_line(i);
  }

  return local_sums;
}

void print_batch(int line_cnt, int batch_cnt)
{
  int i;

  for(i = 1; i < line_cnt; i++)
  {
    int diff = sums[i-1] - sums[i];
    printf(
      "%d - %d: %d\n",
      i-1 + (batch_cnt * MAX_BATCH_SIZE),
      i + (batch_cnt * MAX_BATCH_SIZE),
      diff
    );

  }
}

int read_lines(FILE * fp){
  int line_cnt = 0;
  size_t max_line_length = MAX_LINE_LENGTH * sizeof(char);

  while ((line_lens[line_cnt] = getline((char**) &lines[line_cnt], &max_line_length, fp)) != -1) {
    if(max_line_length != MAX_LINE_LENGTH)
    {
      printf("Line len err: %d\n", max_line_length);
      exit(EXIT_FAILURE);
    }
    line_cnt++;

    if(line_cnt >= MAX_BATCH_SIZE)
      break;
  }
  return line_cnt;
}

int main(int argc, char * argv[])
{
  int i;
  int line_cnt = MAX_BATCH_SIZE;
  int batch_cnt = 0;
  void * status;
  long * local_sums;

  FILE * fp;
  FILE * out;
  char * line = NULL;

  double summation_time = 0;
  struct timeval start, end;
  char * perf_out;

  int rank, size;

  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if( argc > 2)
  {
    thread_count = atoi(argv[1]);
    perf_out = argv[2];
  }
  else
  {
    thread_count = DEFAULT_THREAD_COUNT;
    char * perf_out = "performance.csv";
  }

  if(rank == 0)
  {
    fp = fopen("/homes/dan/625/wiki_dump.txt", "r");

    if (fp == NULL) {
      printf("Error opening\n");
      exit(EXIT_FAILURE);
    }

    /* Local sum blocks are rounded up so add thread_count to be safe */
    /* Only root needs sums */
    sums = malloc( sizeof(*sums) * (thread_count + MAX_BATCH_SIZE));
  }

  while(line_cnt >= MAX_BATCH_SIZE)
  {
    if(rank == 0)
    {
      line_cnt = read_lines(fp);

      /* Round block size up to cover MAX_BATCH_SIZE */
      block_size = (line_cnt / thread_count) + 1;
    }

    MPI_Bcast(
      &block_size,
      1,
      MPI_INT,
      0,
      MPI_COMM_WORLD
    );

    MPI_Bcast(
      &lines,
      MAX_BATCH_SIZE * MAX_LINE_LENGTH,
      MPI_CHAR,
      0,
      MPI_COMM_WORLD
    );

    gettimeofday(&start, NULL);
    local_sums = run_thread(rank);
    gettimeofday(&end, NULL);


    MPI_Gather(
      local_sums,
      block_size,
      MPI_LONG,

      sums,
      block_size,
      MPI_LONG,

      0,
      MPI_COMM_WORLD
    );

    free(local_sums);

    summation_time += end.tv_sec + end.tv_usec / 1e6 -
                      start.tv_sec - start.tv_usec / 1e6;

    if (rank == 0)
    {
      print_batch(line_cnt, batch_cnt);
    }

    batch_cnt++;
  }

  fclose(fp);
  MPI_Finalize();

  out = fopen(perf_out, "a");
  fprintf(out, "%d, %f\n", thread_count, summation_time);

  fclose(out);
}