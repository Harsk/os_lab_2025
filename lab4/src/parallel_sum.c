#include <getopt.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "sum.h"
#include "utils.h"  // GenerateArray из лабы 3

void *ThreadSum(void *args) {
  struct SumArgs *a = (struct SumArgs *)args;
  a->sum = Sum(a);
  return NULL;
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0, array_size = 0, seed = 0;

  static struct option options[] = {{"threads_num", required_argument, 0, 0},
                                    {"seed", required_argument, 0, 0},
                                    {"array_size", required_argument, 0, 0},
                                    {0, 0, 0, 0}};
  int idx = 0, c;
  while ((c = getopt_long(argc, argv, "", options, &idx)) != -1) {
    if (c != 0) return 1;
    switch (idx) {
      case 0: threads_num = atoi(optarg); break;
      case 1: seed = atoi(optarg); break;
      case 2: array_size = atoi(optarg); break;
    }
  }
  if (threads_num == 0 || array_size == 0) {
    printf("Usage: %s --threads_num N --seed N --array_size N\n", argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);  // не входит в замер времени

  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];

  struct timeval start, finish;
  gettimeofday(&start, NULL);

  uint32_t chunk = array_size / threads_num;
  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = i * chunk;
    args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk;
    args[i].sum = 0;
    if (pthread_create(&threads[i], NULL, ThreadSum, &args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  long long total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    pthread_join(threads[i], NULL);
    total_sum += args[i].sum;
  }

  gettimeofday(&finish, NULL);
  double ms = (finish.tv_sec - start.tv_sec) * 1000.0 +
              (finish.tv_usec - start.tv_usec) / 1000.0;

  free(array);
  printf("Total: %lld\nTime: %.3f ms\n", total_sum, ms);
  return 0;
}
