#pragma once

#include <hip/hip_runtime.h>

#include <cstdio>
#include <cstdlib>

#define HIP_CHECK(expr)                                                                      \
    do {                                                                                     \
        hipError_t hl_err_ = (expr);                                                         \
        if (hl_err_ != hipSuccess) {                                                         \
            std::fprintf(stderr, "HIP error %s at %s:%d: %s\n", hipGetErrorName(hl_err_),    \
                         __FILE__, __LINE__, hipGetErrorString(hl_err_));                    \
            std::exit(1);                                                                    \
        }                                                                                    \
    } while (0)
