#include <cuda.h>
#include "utils.h"

void test_memop_ping() {
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

    // Create and record Event A
    CUevent eventA;
    CHECK_CUDA(cuEventCreate(&eventA, 0));
    CHECK_CUDA(cuEventRecord(eventA, stream));

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

    // Allocate large source and destination device buffers (e.g., 1 GiB)
    size_t copy_bytes = 1ULL * 1024 * 1024 * 1024;
    CUdeviceptr src_ptr, dst_ptr;
    CHECK_CUDA(cuMemAlloc(&src_ptr, copy_bytes));
    CHECK_CUDA(cuMemAlloc(&dst_ptr, copy_bytes));

    // Initialize source buffer
    CHECK_CUDA(cuMemsetD8Async(src_ptr, 0xAB, copy_bytes, stream));

    CHECK_CUDA(cuMemcpyDtoDAsync(dst_ptr, src_ptr, copy_bytes, stream));

    // Create and record Event B
    CUevent eventB;
    CHECK_CUDA(cuEventCreate(&eventB, 0));
    CHECK_CUDA(cuEventRecord(eventB, stream));

    CHECK_CUDA(cuEventDestroy(eventA));
    CHECK_CUDA(cuEventDestroy(eventB));
    CHECK_CUDA(cuMemFree(dst_ptr));
    CHECK_CUDA(cuMemFree(src_ptr));
    CHECK_CUDA(cuMemFree(flag_ptr));
    CHECK_CUDA(cuStreamDestroy(stream));
    CHECK_CUDA(cuCtxDestroy(context));
}

int main() {
  test_memop_ping();
  return 0;
}