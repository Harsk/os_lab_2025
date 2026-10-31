#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>

// Структура для передачи аргументов в каждый поток
typedef struct {
    int start;
    int end;
    int mod;
} ThreadArgs;

// Глобальные переменные для хранения общего результата и мьютекса
long long total_result = 1;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

// Функция, которую выполняет каждый поток
void* multiply_subrange(void* arguments) {
    ThreadArgs* args = (ThreadArgs*)arguments;
    long long local_result = 1;

    // Вычисляем произведение на назначенном потоку отрезке
    for (int i = args->start; i <= args->end; i++) {
        local_result = (local_result * i) % args->mod;
        if (local_result == 0) break; // Оптимизация: если локально получили 0, дальше считать нет смысла
    }

    // Критическая секция: безопасно умножаем локальный результат на общий
    pthread_mutex_lock(&result_mutex);
    total_result = (total_result * local_result) % args->mod;
    pthread_mutex_unlock(&result_mutex);

    free(args); // Освобождаем память, выделенную под структуру в main
    return NULL;
}

int main(int argc, char* argv[]) {
    int k = -1;
    int pnum = -1;
    int mod = -1;

    // Настройка парсинга параметров командной строки
    static struct option long_options[] = {
        {"pnum", required_argument, 0, 'p'},
        {"mod",  required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "k:p:m:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'k':
                k = atoi(optarg);
                break;
            case 'p': // Обрабатывает и -p, и --pnum
                pnum = atoi(optarg);
                break;
            case 'm': // Обрабатывает и -m, и --mod
                mod = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Использование: %s -k <число> --pnum=<потоки> --mod=<модуль>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Проверка корректности введенных данных
    if (k < 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "Ошибка: Все параметры (-k, --pnum, --mod) обязательны и должны быть положительными.\n");
        exit(EXIT_FAILURE);
    }

    // Если потоков запрошено больше, чем чисел, уменьшаем их количество
    if (pnum > k) {
        pnum = k;
    }

    pthread_t* threads = malloc(pnum * sizeof(pthread_t));
    
    // Равномерно распределяем работу между потоками
    int base_chunk = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;

    for (int i = 0; i < pnum; i++) {
        ThreadArgs* args = malloc(sizeof(ThreadArgs));
        args->start = current_start;
        
        // Распределяем остаток от деления между первыми потоками
        int chunk_size = base_chunk + (i > remainder ? 1 : 0);
        args->end = current_start + chunk_size - 1;
        args->mod = mod;

        current_start = args->end + 1;

        // Создаем поток
        if (pthread_create(&threads[i], NULL, multiply_subrange, (void*)args) != 0) {
            perror("Ошибка pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // Ожидаем завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Ошибка pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    // Выводим финальный результат
    printf("Результат (%d! mod %d) = %lld\n", k, mod, total_result);

    // Очистка ресурсов
    free(threads);
    pthread_mutex_destroy(&result_mutex);

    return 0;
}
