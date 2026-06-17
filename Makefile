# ==========================================
# Compilers Configuration
# ==========================================
HOST_CXX = g++
RV_CXX   = riscv64-unknown-elf-g++

# ==========================================
# Dynamic Optimization & Profiling Flags
# ==========================================
# Default optimization is -O3. Change it in terminal like: make canny_host OPT=-O0
OPT ?= -O3

# Flag to print auto-vectorization report (useful with -O3 or -Ofast)
VEC_REPORT = -fopt-info-vec-all

# ==========================================
# Compilation Flags
# ==========================================
HOST_FLAGS = -I$(GTEST_ROOT)/include -Iinclude $(OPT) $(VEC_REPORT)
HOST_LIBS  = -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -lpthread -lm

RV_FLAGS   = -Iinclude -march=rv64gcv $(OPT) $(VEC_REPORT)
RV_LIBS    = -lm

# ==========================================
# Source Files
# ==========================================
ALL_SRCS  = $(wildcard src/*.cpp)
SRC_FILES = $(filter-out src/main.cpp, $(ALL_SRCS))
TEST_SRCS = $(wildcard tests/*.cpp)

# ==========================================
# Build Targets
# ==========================================

# 1. TEST: Compiles and runs all GoogleTest suites natively on the host
test:
	@mkdir -p bin
	$(HOST_CXX) $(HOST_FLAGS) $(SRC_FILES) $(TEST_SRCS) -o bin/unit_tests $(HOST_LIBS)
	./bin/unit_tests

# 2. CANNY_HOST: Compiles the main pipeline natively for the host (Used by Python script)
canny_host:
	@mkdir -p bin
	$(HOST_CXX) $(HOST_FLAGS) $(SRC_FILES) src/main.cpp -o bin/canny_app -lm
	@echo "\n=== HOST BINARY SIZE ($(OPT)) ==="
	@size bin/canny_app

# 3. CANNY_RV: Cross-compiles the pipeline for RISC-V target
canny_rv:
	@mkdir -p bin_rv
	$(RV_CXX) $(RV_FLAGS) $(SRC_FILES) src/main.cpp -o bin_rv/canny_riscv $(RV_LIBS)
	@echo "\n=== RISC-V BINARY SIZE ($(OPT)) ==="
	@riscv64-unknown-elf-size bin_rv/canny_riscv

# 4. RUN: Executes the compiled RISC-V binary on QEMU
run: canny_rv
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin_rv/canny_riscv 512 512 1 50 150

# 5. CLEAN: Removes generated binaries
clean:
	rm -rf bin bin_rv

.PHONY: test canny_host canny_rv run clean