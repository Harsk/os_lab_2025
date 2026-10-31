#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> // Нужен для функции sleep()

// Создаем два навесных замка (мьютекса)
pthread_mutex_t mutex_A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_B = PTHREAD_MUTEX_INITIALIZER;

// Функция для первого потока
void* thread_one_worker(void* arg) {
    printf("[Поток 1]: Пытаюсь захватить замок А...\n");
    pthread_mutex_lock(&mutex_A);
    printf("[Поток 1]: Успешно захватил замок А! Сплю 1 секунду...\n");
    
    // Искусственная пауза, чтобы Поток 2 успел перехватить замок Б
    sleep(1);

    printf("[Поток 1]: Теперь пытаюсь захватить замок Б...\n");
    pthread_mutex_lock(&mutex_B); // ЗДЕСЬ ПОТОК 1 ЗАВИСНЕТ НАВСЕГДА
    
    printf("[Поток 1]: Ура! Я захватил оба замка!\n");
    
    pthread_mutex_unlock(&mutex_B);
    pthread_mutex_unlock(&mutex_A);
    return NULL;
}

// Функция для второго потока
void* thread_two_worker(void* arg) {
    printf("[Поток 2]: Пытаюсь захватить замок Б...\n");
    pthread_mutex_lock(&mutex_B);
    printf("[Поток 2]: Успешно захватил замок Б! Сплю 1 секунду...\n");
    
    // Искусственная пауза, чтобы Поток 1 успел перехватить замок А
    sleep(1);

    printf("[Поток 2]: Теперь пытаюсь захватить замок А...\n");
    pthread_mutex_lock(&mutex_A); // ЗДЕСЬ ПОТОК 2 ЗАВИСНЕТ НАВСЕГДА
    
    printf("[Поток 2]: Ура! Я захватил оба замка!\n");
    
    pthread_mutex_unlock(&mutex_A);
    pthread_mutex_unlock(&mutex_B);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    printf("[Главный поток]: Запускаю два потока-соперника...\n");

    pthread_create(&thread1, NULL, thread_one_worker, NULL);
    pthread_create(&thread2, NULL, thread_two_worker, NULL);

    // Главный поток будет вечно ждать их завершения на этих строках
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("[Главный поток]: Программа успешно завершена (вы никогда не увидите этот текст).\n");
    return 0;
}
