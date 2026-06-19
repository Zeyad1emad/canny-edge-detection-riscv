# =========================================================================
# COMPILER CONFIGURATIONS & FLAGS (UNCHANGED)
# =========================================================================
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

.PHONY: all clean host_tests rvv_tests test run_host run_rvv

# Global Entry Points
all: run_host run_rvv host_tests rvv_tests
test: host_tests rvv_tests

# =========================================================================
# BLOCK 1: HOST SIDE TARGETS (PC / SCALAR / GOOGLETEST)
# =========================================================================

# 1. Host Main Pipeline Executable
run_host: src/main.cpp $(HOST_SRC_FILES)
	@echo "Building Standalone Host Pipeline Executable..."
	$(HOST_CXX) $(HOST_FLAGS) $^ -o pipeline_host.out $(HOST_LIBS)

# 2. Master target to compile and run all 5 Host-side tests
host_tests: test_gaussian_host test_direction_host test_magnitude_host test_sobel_host test_main_host
	@echo "Running Host-side GoogleTests..."
	./test_gaussian_host
	./test_direction_host
	./test_magnitude_host
	./test_sobel_host
	./test_main_host

# Individual Host Test Compilations
test_gaussian_host: tests/test_gaussian.cpp $(HOST_SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_direction_host: tests/test_direction.cpp $(HOST_SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_magnitude_host: tests/test_magnitude.cpp $(HOST_SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_sobel_host: tests/test_sobel.cpp $(HOST_SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_main_host: tests/test_main.cpp $(HOST_SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)


# =========================================================================
# BLOCK 2: RVV SIDE TARGETS (RISC-V VECTOR / QEMU)
# =========================================================================

# 1. RVV Main Pipeline Executable
VLEN ?= 256

run_rvv: src/main_rvv.cpp $(RVV_SRC_FILES)
	@echo "Building Standalone RVV Pipeline Executable with VLEN=$(VLEN)..."
	$(RV_CXX) $(RV_FLAGS) -march=rv64gcv_zvl$(VLEN)b -DRVV_VLEN=$(VLEN) $^ -o pipeline_rvv.out $(RV_LIBS)
# 2. Master target to compile and run all RVV-side tests on QEMU
rvv_tests: test_equiv_rvv test_magnitude_rvv
	@echo "Running RISC-V Vector Tests on QEMU..."
	qemu-riscv64 ./test_equiv_rvv
	qemu-riscv64 ./test_magnitude_rvv

# Individual RVV Test Compilations
test_gaussian_rvv: tests/test_gaussian_rvv.cpp $(RVV_SRC_FILES)
	$(RV_CXX) $(RV_FLAGS) $^ -o $@ $(RV_LIBS)
test_sobel_rvv: tests/test_sobel_rvv.cpp $(RVV_SRC_FILES)
	$(RV_CXX) $(RV_FLAGS) $^ -o $@ $(RV_LIBS)

test_magnitude_rvv: tests/test_magnitude_rvv.cpp $(RVV_SRC_FILES)
	$(RV_CXX) $(RV_FLAGS) $^ -o $@ $(RV_LIBS)


# =========================================================================
# CLEANUP
# =========================================================================
clean:
	rm -f test_gaussian_host test_direction_host test_magnitude_host test_sobel_host test_main_host
	rm -f test_equiv_rvv test_magnitude_rvv pipeline_host.out pipeline_rvv.out