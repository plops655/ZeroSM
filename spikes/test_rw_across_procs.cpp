#include <cstring>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "utils.h"
#include <unistd.h>
#include <cassert>

void send_fd(int send_socket, int fd) {
    alignas(cmsghdr) char buffer[CMSG_SPACE(sizeof(int))] = {};

    iovec iov{};
    iov.iov_base = buffer;          
    iov.iov_len = sizeof(buffer);  

    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = fd;          

    cmsghdr *hdr = CMSG_FIRSTHDR(&msg);
    hdr->cmsg_level = SOL_SOCKET;
    hdr->cmsg_type = SCM_RIGHTS;
    hdr->cmsg_len = CMSG_LEN(sizeof(int));

    memcpy(CMSG_DATA(hdr), &fd, sizeof(int));

    // Send fd over socket
    sendmsg(send_socket, &msg, 0);
}

ssize_t recv_fd(int recv_socket, int fd) {
     alignas(cmsghdr) char buffer[CMSG_SPACE(sizeof(int))] = {};

    iovec iov{};
    iov.iov_base = buffer;          
    iov.iov_len = sizeof(buffer);  

    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = fd;   

    ssize_t n = recvmsg(recv_socket, &msg, 0);
    if (n > 0) {
        if (msg.msg_flags && MSG_CTRUNC) {
            // error
        }
        cmsghdr* hdr = CMSG_FIRSTHDR(&msg);
        assert(hdr != nullptr);
        assert(hdr->cmsg_level == SOL_SOCKET);
        assert(hdr->cmsg_type == SCM_RIGHTS);
        
    }
}

void test_rw_across_procs() {
    int sv[2];
    CHECK_SYS(socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
    int fd = memfd_create("inter-process", 0);
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
        write(fd, buffer, 20);
        close(sv[0]);
        int status;
        CHECK_SYS(waitpid(pid, &status, 0));
    }
}

int main() {
    test_rw_across_procs();
    return 0;
}