#include"HTTPServer.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void run_server_child() {
    HTTPServer *server = HTTPServer_create(8080);

    ThreadPool *pool = ThreadPool_create(8);

    while (1) {
        HTTPRequest req = HTTPServer_listen(server);
        ThreadPool_push(pool, req);
    }
}

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // Child
        run_server_child();
        return 0;
    }

    // Parent supervisor
    printf("Server running in child PID %d\n", pid);

    int status;
    waitpid(pid, &status, 0);
    printf("Child exited, status %d\n", status);

    return 0;
}

