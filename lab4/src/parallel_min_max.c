#include <ctype.h>
#include <limits.h>
///////
#include <signal.h>
////////
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"
/////////чтоб не ругалось на отсутствие
volatile sig_atomic_t timed_out = 0;

void on_alarm(int sig) {
  (void)sig;
  timed_out = 1;
}
//////////
int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
/////////
  int timeout = 0;  // 0 = без таймаута
/////////
  bool with_files = false;
  int (*pipes_ptr)[2] = NULL;

  while (true) {
    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},//////////
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be positive\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be positive\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be positive\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
////////////проверка вводимого
          case 4:
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("timeout must be positive\n");
              return 1;
            }
            break;
////////////
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" "
           "[--by_files] [--timeout \"sec\"]\n",//////////////
           argv[0]);
    return 1;
  }

  // пайпы создаём, когда уже известны pnum и with_files
  if (!with_files) {
    pipes_ptr = malloc(sizeof(int[2]) * pnum);
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes_ptr[i]) < 0) {
        perror("pipe");
        return 1;
      }
    }
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;
  //////////////////выделяем
  pid_t *child_pids = malloc(sizeof(pid_t) * pnum);
/////////////////////////
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
//////////////////////
      child_pids[i] = child_pid;
//////////////////////
      if (child_pid == 0) {
        // child process
        unsigned int step = array_size / pnum;
        unsigned int begin = i * step;
        unsigned int end = (i == pnum - 1) ? array_size : (i + 1) * step;
        struct MinMax local_min_max = GetMinMax(array, begin, end);

        if (with_files) {
          char file_name[64];
          sprintf(file_name, "result_%d.txt", i);
          FILE *fp = fopen(file_name, "w");
          if (fp) {
            fprintf(fp, "%d %d\n", local_min_max.min, local_min_max.max);
            fclose(fp);
          }
        } else {
          close(pipes_ptr[i][0]);
          write(pipes_ptr[i][1], &local_min_max, sizeof(struct MinMax));
          close(pipes_ptr[i][1]);
        }
        free(array);
        free(child_pids);
        if (pipes_ptr) free(pipes_ptr);
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  //////////// ожидание детей: неблокирующее, с учётом таймаута
  if (timeout > 0) {
    signal(SIGALRM, on_alarm);
    alarm(timeout);
  }
  bool killed = false;
  while (active_child_processes > 0) {
    pid_t r = waitpid(-1, NULL, WNOHANG);
    if (r > 0) {
      active_child_processes -= 1;
    } else if (r == 0) {
      if (timed_out && !killed) {
        for (int i = 0; i < pnum; i++) kill(child_pids[i], SIGKILL);
        killed = true;
        printf("Timeout: children killed\n");
      }
      usleep(1000);
    } else {
      break;
    }
    ////////////////////
  }
  free(child_pids);//////////////

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char file_name[64];
      sprintf(file_name, "result_%d.txt", i);
      FILE *fp = fopen(file_name, "r");
      if (fp) {
        fscanf(fp, "%d %d", &min, &max);
        fclose(fp);
        remove(file_name);
      }
    } else {
        ////////////////////
      struct MinMax local_min_max;
      close(pipes_ptr[i][1]);
      ssize_t n = read(pipes_ptr[i][0], &local_min_max, sizeof(struct MinMax));
      close(pipes_ptr[i][0]);
      if (n == (ssize_t)sizeof(struct MinMax)) {
        min = local_min_max.min;
        max = local_min_max.max;
      }
      /////////////////////////
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  if (pipes_ptr) free(pipes_ptr);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
