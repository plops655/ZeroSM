#include <cstring>
#include <sys/socket.h>
#include <sys/wait.h>
#include "utils.h"
#include <unistd.h>


void test_rw_across_procs() {
    int sv[2];
    CHECK_SYS(socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
    pid_t pid = fork();
    if (pid == -1) {
        return;
    }
    // Child Process
    if (pid == 0) {
        close(sv[0]);
        char byte = 'h';
        write(sv[1], &byte, 1);
        close(sv[1]);
        _exit(0);
    } else {
        close(sv[1]);
        char byte;
        ssize_t n = read(sv[0], &byte, 1);
        if (n == 1) {
            printf("Read byte %c", byte);
        }
        close(sv[0]);
        int status;
        CHECK_SYS(waitpid(pid, &status, 0));
    }
}

int main() {
    test_rw_across_procs();
    return 0;
}