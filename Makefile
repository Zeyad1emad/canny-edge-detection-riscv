HOST_CXX ?= g++
RV_CXX   ?= riscv64-unknown-elf-g++

OPT        ?= -O3
VEC_REPORT ?= 

GTEST_ROOT ?= /usr/local
HOST_FLAGS  = -I$(GTEST_ROOT)/include -Iinclude -Isrc $(OPT) $(VEC_REPORT)
HOST_LIBS   = -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -lpthread -lm

RV_FLAGS    = -Iinclude -Isrc -march=rv64gcv $(OPT) $(VEC_REPORT)
RV_LIBS     = -lm

# Exclude all main files and RVV files from Host builds
HOST_SRC_FILES = $(filter-out src/main.cpp src/main_bench.cpp src/main_rvv.cpp src/%_rvv.cpp, $(wildcard src/*.cpp))

# Exclude all main files from RVV builds
RVV_SRC_FILES  = $(filter-out src/main.cpp src/main_bench.cpp src/main_rvv.cpp, $(wildcard src/*.cpp))

.PHONY: all clean host_tests rvv_tests test run_rvv

all: host_tests rvv_tests run_rvv

test: host_tests rvv_tests

host_tests: test_gaussian_host test_magnitude_host test_sobel_host test_direction_host
	@echo "Running Host-side GoogleTests..."
	./test_gaussian_host
	./test_magnitude_host
	./test_sobel_host
	./test_direction_host

rvv_tests: test_gaussian_rvv
	@echo "Running RISC-V Vector Tests on QEMU..."
	qemu-riscv64 ./test_gaussian_rvv

# Build Target for the Full RVV Pipeline Executable
run_rvv: src/main_rvv.cpp $(RVV_SRC_FILES)
	@echo "Building Standalone RVV Pipeline Executable..."
	$(RV_CXX) $(RV_FLAGS) $^ -o pipeline_rvv.out $(RV_LIBS)

test_gaussian_host: tests/test_gaussian.cpp $(HOST_SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_magnitude_host: tests/test_magnitude.cpp $(HOST_SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_sobel_host: tests/test_sobel.cpp $(HOST_SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_direction_host: tests/test_direction.cpp $(HOST_SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_gaussian_rvv: tests/test_gaussian_rvv.cpp $(RVV_SRC_FILES)
	$(RV_CXX) $(RV_FLAGS) $^ -o $@ $(RV_LIBS)

clean:
	rm -f test_gaussian_host test_magnitude_host test_sobel_host test_direction_host test_gaussian_rvv pipeline_rvv.out