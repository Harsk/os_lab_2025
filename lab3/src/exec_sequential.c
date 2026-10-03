#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0) {
        //Доч проц: запускаем sequential_min_max
        //Арг: имя, seed, arraysize, и завершающий NULL
        char *args[] = {"./sequential_min_max", "42", "10", NULL};
        
        printf("[Child] Executing sequential_min_max via execv...\n");
        execv(args[0], args);
        

        //execv возвращает управление
        perror("Execv failed");
        exit(1);
    } else {
        // Род проц: ждем завершения дочернего
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("[Parent] Child process exited with status %d\n", WEXITSTATUS(status));
        } else {
            printf("[Parent] Child process terminated abnormally\n");
        }
    }

    return 0;
}
