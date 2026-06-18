# ==========================================
# Compilers Configuration
# ==========================================
HOST_CXX = g++
RV_CXX   = riscv64-unknown-elf-g++

# ==========================================
# Dynamic Optimization & Profiling Flags
# ==========================================
OPT ?= -O3
VEC_REPORT = -fopt-info-vec-all

# ==========================================
# Compilation Flags
# ==========================================
HOST_FLAGS = -I$(GTEST_ROOT)/include -Iinclude -Isrc $(OPT) $(VEC_REPORT)
HOST_LIBS  = -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -lpthread -lm

RV_FLAGS   = -Iinclude -Isrc -march=rv64gcv $(OPT) $(VEC_REPORT)
RV_LIBS    = -lm

# ==========================================
# Source Files Filtering (CRITICAL)
# ==========================================
ALL_SRCS  = $(wildcard src/*.cpp)

# Host compiler (g++) cannot understand <riscv_vector.h>, so we exclude sobel_rvv.cpp
HOST_SRCS = $(filter-out src/main.cpp src/sobel_rvv.cpp, $(ALL_SRCS))

# RISC-V compiler gets everything (including sobel_rvv.cpp)
RV_SRCS   = $(filter-out src/main.cpp, $(ALL_SRCS))
TEST_SRCS = $(wildcard tests/*.cpp)

# ==========================================
# Build Targets
# ==========================================

# 1. TEST: Runs GTest for normal scalar code on host
test:
	@mkdir -p bin
	$(HOST_CXX) $(HOST_FLAGS) $(HOST_SRCS) $(TEST_SRCS) -o bin/unit_tests $(HOST_LIBS)
	./bin/unit_tests

# 2. CANNY_HOST: Host binary
canny_host:
	@mkdir -p bin
	$(HOST_CXX) $(HOST_FLAGS) $(HOST_SRCS) src/main.cpp -o bin/canny_app -lm
	@echo "\n=== HOST BINARY SIZE ($(OPT)) ==="
	@size bin/canny_app

# 3. CANNY_RV: RISC-V binary
canny_rv:
	@mkdir -p bin_rv
	$(RV_CXX) $(RV_FLAGS) $(RV_SRCS) src/main.cpp -o bin_rv/canny_riscv $(RV_LIBS)
	@echo "\n=== RISC-V BINARY SIZE ($(OPT)) ==="
	@riscv64-unknown-elf-size bin_rv/canny_riscv

# 4. RUN: Executes RISC-V on QEMU
run: canny_rv
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin_rv/canny_riscv 512 512 1 50 150

# 5. TEST_RV: Special target to test RVV code correctly on QEMU (Phase 6 Equivalence Test)
test_rv:
	@mkdir -p bin_rv
	$(RV_CXX) $(RV_FLAGS) $(RV_SRCS) tests/test_equiv.cpp -o bin_rv/test_equiv_app $(RV_LIBS)
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin_rv/test_equiv_app

# 6. CLEAN
clean:
	rm -rf bin bin_rv

bench:
	riscv64-unknown-elf-g++ -O3 -march=rv64gcv src/main_bench.cpp src/sobel.cpp src/sobel_rvv.cpp -o bin/bench_app
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin/bench_app

.PHONY: test canny_host canny_rv run test_rv clean bench