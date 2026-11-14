#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
 
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
 
#include "mult_modulo.h"
 
struct Server {
  char ip[255];
  int port;
};
 
bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }
 
  if (errno != 0)
    return false;
 
  *val = i;
  return true;
}
 
int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'}; // 255 байт - максимальная длина имени файла в Linux (NAME_MAX)
 
  while (true) {
    int current_optind = optind ? optind : 1;
 
    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};
 
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
 
    if (c == -1)
      break;
 
    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        // TODO: your code here
        if (k == 0) {
          fprintf(stderr, "k must be positive\n");
          return 1;
        }
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        // TODO: your code here
        if (mod == 0) {
          fprintf(stderr, "mod must be positive\n");
          return 1;
        }
        break;
      case 2:
        // TODO: your code here
        if (strlen(optarg) >= sizeof(servers)) {
          fprintf(stderr, "Path to servers file is too long\n");
          return 1;
        }
        memcpy(servers, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;
 
    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }
 
  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }
 
  // TODO: for one server here, rewrite with servers from file
  // Файл servers: в каждой строке ip:port. Сначала считаем строки...
  unsigned int servers_num = 0;
  char ip_buf[255];
  int port_buf;
  FILE *file = fopen(servers, "r");
  if (file == NULL) {
    fprintf(stderr, "Can not open file %s\n", servers);
    return 1;
  }
  while (fscanf(file, " %254[^:]:%d", ip_buf, &port_buf) == 2)
    servers_num++;
  if (servers_num == 0) {
    fprintf(stderr, "No servers found in %s\n", servers);
    fclose(file);
    return 1;
  }
 
  // ...потом читаем их в массив
  struct Server *to = malloc(sizeof(struct Server) * servers_num);
  rewind(file);
  for (unsigned int i = 0; i < servers_num; i++) {
    if (fscanf(file, " %254[^:]:%d", to[i].ip, &to[i].port) != 2)
      break;
  }
  fclose(file);
  // TODO: delete this and parallel work between servers
  // (жёстко заданный сервер 127.0.0.1:20001 удалён)
 
  int *sockets = malloc(sizeof(int) * servers_num);
 
  // TODO: work continiously, rewrite to make parallel
  // Этап 1: подключаемся к каждому серверу и сразу отправляем ему задачу,
  // не дожидаясь ответа. Поэтому все сервера считают одновременно.
  for (unsigned int i = 0; i < servers_num; i++) {
    struct hostent *hostname = gethostbyname(to[i].ip);
    if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", to[i].ip);
      exit(1);
    }
 
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(to[i].port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);
 
    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      exit(1);
    }
 
    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
      fprintf(stderr, "Connection failed\n");
      exit(1);
    }
 
    // TODO: for one server
    // parallel between servers
    // делим числа 1..k поровну между серверами, последний берёт остаток
    uint64_t chunk = k / servers_num;
    uint64_t begin = i * chunk + 1;
    uint64_t end = (i == servers_num - 1) ? k : (i + 1) * chunk;
 
    char task[sizeof(uint64_t) * 3];
    memcpy(task, &begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &mod, sizeof(uint64_t));
 
    if (send(sck, task, sizeof(task), 0) < 0) {
      fprintf(stderr, "Send failed\n");
      exit(1);
    }
 
    sockets[i] = sck;
  }
 
  // Этап 2: собираем ответы от всех серверов
  uint64_t answer = 1;
  for (unsigned int i = 0; i < servers_num; i++) {
    char response[sizeof(uint64_t)];
    if (recv(sockets[i], response, sizeof(response), 0) < 0) {
      fprintf(stderr, "Recieve failed\n");
      exit(1);
    }
 
    // TODO: from one server
    // unite results
    uint64_t part = 0;
    memcpy(&part, response, sizeof(uint64_t));
    answer = MultModulo(answer, part, mod);
 
    close(sockets[i]);
  }
  printf("answer: %llu\n", answer);
  free(sockets);
  free(to);
 
  return 0;
}
