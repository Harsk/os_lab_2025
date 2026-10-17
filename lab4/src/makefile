CC = gcc
CFLAGS = -Wall -O2

all: parallel_min_max process_memory

parallel_min_max: parallel_min_max.c utils.o find_min_max.o
	$(CC) $(CFLAGS) $^ -o $@

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

find_min_max.o: find_min_max.c find_min_max.h
	$(CC) $(CFLAGS) -c find_min_max.c

process_memory: process_memory.c
	$(CC) -std=gnu89 -w process_memory.c -o $@

clean:
	rm -f *.o parallel_min_max process_memory

.PHONY: all clean

# •	Как появляются: ребёнок завершился, а родитель жив и не вызвал wait или waitpid. Ядро хранит запись о ребёнке (PID, код завершения), чтобы родитель мог её прочитать. Память и другие ресурсы ребёнок уже освободил.
# •	Чем опасны: занимают записи в таблице процессов. Если их накопится много, закончатся PID, и fork начнёт возвращать ошибку.
# •	Как избавиться: вызывать wait или waitpid в родителе (в том числе в обработчике SIGCHLD); установить signal(SIGCHLD, SIG_IGN); либо завершить родителя, тогда осиротевших зомби усыновит init и сразу соберёт. Убить сам зомби через kill нельзя, он уже мёртв.
