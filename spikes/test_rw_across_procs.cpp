#include <cstring>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "utils.h"
#include <unistd.h>
#include <cuda.h>

int init_cuda() {
    // Initialize cuda driver API
    CHECK_CUDA(cuInit(0));

    int count;
    CHECK_CUDA(cuDeviceGetCount(&count));

    if (count == 0) {
        printf("No GPUs available");
        return -1;
    }
    // Get the cuda device and context
    CUdevice device;
    CHECK_CUDA(cuDeviceGet(&device, 0));

    int pi1, pi2;
    CHECK_CUDA(cuDeviceGetAttribute(&pi1, CU_DEVICE_ATTRIBUTE_VIRTUAL_MEMORY_MANAGEMENT_SUPPORTED, device));
    CHECK_CUDA(cuDeviceGetAttribute(&pi2, CU_DEVICE_ATTRIBUTE_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR_SUPPORTED, device));
    if (pi1 == 0 || pi2 == 0) {
        printf("CUDA device attributs incorrect.");
        return -1;
    }

    CUcontext context;
    cuDevicePrimaryCtxRetain(&context, device);
    cuCtxSetCurrent(context);
    return 0;
}

void send_fd(int send_socket, int fd) {
    alignas(cmsghdr) char buffer[CMSG_SPACE(sizeof(int))] = {};

    char byte = 'h';

    iovec iov{};
    iov.iov_base = &byte;          
    iov.iov_len = 1;  

    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;      
    msg.msg_control = buffer;
    msg.msg_controllen = sizeof(buffer);    

    cmsghdr *hdr = CMSG_FIRSTHDR(&msg);
    hdr->cmsg_level = SOL_SOCKET;
    hdr->cmsg_type = SCM_RIGHTS;
    hdr->cmsg_len = CMSG_LEN(sizeof(int));

    memcpy(CMSG_DATA(hdr), &fd, sizeof(int));

    // Send fd over socket
    CHECK_SYS(sendmsg(send_socket, &msg, 0));
}

int recv_fd(int recv_socket) {
    // Make empty control buffer
    alignas(cmsghdr) char buffer[CMSG_SPACE(sizeof(int))] = {};
    
    char byte;

    // dummy payload
    iovec iov{};
    iov.iov_base = &byte;          
    iov.iov_len = 1;  

    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buffer;
    msg.msg_controllen = sizeof(buffer);   

    // Copies 'h' into byte
    // Sees SCM_RIGHTS -> creates fd in child's fd table
    // that points to open file pointed by parent's fd.
    // writes a control header into buffer with type
    // SCM_RIGHTS and new child fd.
    ssize_t n = recvmsg(recv_socket, &msg, 0);
    if (n <= 0 || (msg.msg_flags & MSG_CTRUNC)) {
        fprintf(stderr, "recvmsg failed or control data truncated\n");
        return -1;
    }
    // Fetch the control header and verify no errors
    cmsghdr* hdr = CMSG_FIRSTHDR(&msg);
    if (hdr == nullptr || hdr->cmsg_level != SOL_SOCKET 
    || hdr->cmsg_type != SCM_RIGHTS) {
        fprintf(stderr, "no fd received\n");
        return -1;
    }

    // Fetch new child fd
    int fd;
    memcpy(&fd, CMSG_DATA(hdr), sizeof(int));
    return fd;
}

void test_rw_across_procs() {
    int sv[2];
    CHECK_SYS(socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
    int cuda_init_res = init_cuda();
    if (cuda_init_res == -1) {
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        close(sv[0]);
        close(sv[1]);
        return;
    }
    // Child Process
    if (pid == 0) {
        close(sv[0]);
        int fd = recv_fd(sv[1]);
        if (fd < 0) _exit(1);

        char buffer[20];
        CHECK_SYS(pread(fd, buffer, 20, 0));
        
        close(fd);
        close(sv[1]);
        _exit(0);
    } 
    close(sv[1]);
    int fd = memfd_create("inter-process", 0);
    char buffer[20] = "Hi there!";
    CHECK_SYS(pwrite(fd, buffer, 20, 0));

    send_fd(sv[0], fd);
    close(fd);
    close(sv[0]);

    int status;
    CHECK_SYS(waitpid(pid, &status, 0));
    
}

int main() {
    test_rw_across_procs();
    return 0;
}