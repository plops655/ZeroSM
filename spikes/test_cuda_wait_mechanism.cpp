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

void test_cuStreamWaitValue64() {
    // Initialize cuda driver API
    CHECK_CUDA(cuInit(0));

    // Get the cuda device and context
    CUdevice device;
    CUcontext context;
    CHECK_CUDA(cuDeviceGet(&device, 0));
    CHECK_CUDA(cuCtxCreate(&context, 0, device));

    // Create a CUDA stream
    CUstream stream;
    CHECK_CUDA(cuStreamCreate(&stream, CU_STREAM_DEFAULT));

    // Allocate 64-bit aligned device memory for the flag/semaphore
    CUdeviceptr flag_ptr;
    CHECK_CUDA(cuMemAlloc(&flag_ptr, sizeof(cuuint64_t)));

    // Initialize the memory location to 0
    CHECK_CUDA(cuMemsetD32(flag_ptr, 0, 2));

    // Enqueue the wait operation on the stream:
    // Blocks subsequent commands on `stream` until *flag_ptr >= 1
    cuuint64_t target_value = 1;
    unsigned int flags = CU_STREAM_WAIT_VALUE_GEQ;
    CHECK_CUDA(cuStreamWaitValue64(stream, flag_ptr, target_value, flags));

    printf("Enqueued cuStreamWaitValue64 successfully.\n");
}

int main() {
    test_cuStreamWaitValue64();
}