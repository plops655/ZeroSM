#include <cuda.h>
#include <iostream>

#define CHECK_CUDA(call)                                                       \
  do {                                                                         \
    CUresult err = (call);                                                     \
    if (err != CUDA_SUCCESS) {                                                 \
      const char *errStr = NULL;                                               \
      cuGetErrorString(err, &errStr);                                          \
      fprintf(stderr, "CUDA Error at %s:%d - %s (%d)\n", __FILE__, __LINE__,   \
              errStr ? errStr : "Unknown", err);                               \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define CHECK_SYS(call)                                                         \
  do {                                                                          \
    if ((call) != 0) {                                                          \
      const char *errStr = strerror(errno);                                     \
      fprintf(stderr, "Syscall Error at %s:%d - %s (%d)\n", __FILE__, __LINE__, \
              errStr ? errStr : "Unknown", errno);                                \
      exit(EXIT_FAILURE);                                                       \
    }                                                                           \
  } while (0)