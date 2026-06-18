# ==============================================================================
# Compilers Configuration
# ==============================================================================
HOST_CXX ?= g++
RV_CXX   ?= riscv64-unknown-elf-g++

# ==============================================================================
# Dynamic Optimization & Profiling Flags
# ==============================================================================
OPT        ?= -O3
VEC_REPORT ?= -fopt-info-vec-all

# ==============================================================================
# GoogleTest Paths (Flexible Configuration)
# ==============================================================================
GTEST_ROOT ?= /usr/local
HOST_FLAGS  = -I$(GTEST_ROOT)/include -Iinclude $(OPT) $(VEC_REPORT)
HOST_LIBS   = -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -lpthread -lm

# ==============================================================================
# RISC-V Configuration (No GoogleTest or pthread dependencies)
# ==============================================================================
RV_FLAGS    = -Iinclude -march=rv64gcv $(OPT) $(VEC_REPORT)
RV_LIBS     = -lm

# ==============================================================================
# Targets & Build Rules
# ==============================================================================
.PHONY: all clean host_tests rvv_tests

all: host_tests rvv_tests

# 1. Host-side (Scalar) Tests using GoogleTest
host_tests: test_gaussian_host test_magnitude_host

test_gaussian_host: tests/test_gaussian.cpp src/canny_scalar.cpp
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_magnitude_host: tests/test_magnitude.cpp src/canny_scalar.cpp
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

# 2. RISC-V (Vectorized) Tests running on QEMU (No GTest)
rvv_tests: test_gaussian_rvv test_magnitude_rvv

test_gaussian_rvv: tests/test_gaussian_rvv.cpp src/canny_rvv.cpp
	$(RV_CXX) $(RV_FLAGS) $^ -o $@ $(RV_LIBS)

test_magnitude_rvv: tests/test_magnitude_rvv.cpp src/canny_rvv.cpp
	$(RV_CXX) $(RV_FLAGS) $^ -o $@ $(RV_LIBS)

# ==============================================================================
# Clean Artifacts
# ==============================================================================
clean:
	rm -f test_gaussian_host test_magnitude_host test_gaussian_rvv test_magnitude_rvv