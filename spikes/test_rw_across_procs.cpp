#include <cstring>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "utils.h"
#include <unistd.h>

ssize_t send_fd(int send_socket, int fd) {
    char buffer[1];
    buffer[0] = '0';

    iovec iov{};
    iov.iov_base = buffer;          
    iov.iov_len = sizeof(buffer);  

    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = fd;          

    // Send fd over socket
    return sendmsg(send_socket, &msg, 0);
}

ssize_t recv_fd(int recv_socket, int fd) {
    char buffer[1];
    buffer[0] = '0';
    
    iovec iov{};
    iov.iov_base = buffer;          
    iov.iov_len = sizeof(buffer);  

    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = fd;   

    return recvmsg(recv_socket, &msg, 0);
}

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