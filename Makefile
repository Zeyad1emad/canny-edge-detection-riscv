# ==========================================
# Compilers Configuration
# ==========================================
HOST_CXX = g++
RV_CXX   = riscv64-unknown-elf-g++

# ==========================================
# Compilation Flags
# ==========================================
HOST_FLAGS = -I$(GTEST_ROOT)/include -Iinclude -O3
HOST_LIBS  = -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -lpthread -lm

# Added $(GTEST_ROOT) paths to support GoogleTest with the RISC-V compiler
RV_FLAGS   = -I$(GTEST_ROOT)/include -Iinclude -march=rv64gcv -O3
RV_LIBS    = -L$(GTEST_ROOT)/lib -lm -lgtest -lgtest_main -lpthread

# ==========================================
# Source Files
# ==========================================
ALL_SRCS  = $(wildcard src/*.cpp)
SRC_FILES = $(filter-out src/main.cpp, $(ALL_SRCS))

# Filter out RVV tests from standard host tests to avoid compilation errors
HOST_TEST_SRCS = $(filter-out tests/test_gaussian_rvv.cpp, $(wildcard tests/*.cpp))
RVV_TEST_SRC   = tests/test_gaussian_rvv.cpp

# ==========================================
# Build Targets
# ==========================================

# 1. TEST: Runs only Host-compatible tests
test:
	@mkdir -p bin
	$(HOST_CXX) $(HOST_FLAGS) $(SRC_FILES) $(HOST_TEST_SRCS) -o bin/unit_tests $(HOST_LIBS)
	./bin/unit_tests

# 2. TEST_RVV: Cross-compiles and runs RVV tests on QEMU
test_rvv:
	@mkdir -p bin_rv
	$(RV_CXX) $(RV_FLAGS) $(SRC_FILES) $(RVV_TEST_SRC) -o bin_rv/rvv_unit_tests $(RV_LIBS)
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin_rv/rvv_unit_tests

# 3. CANNY_HOST: Compiles main pipeline natively
canny_host:
	@mkdir -p bin
	$(HOST_CXX) $(HOST_FLAGS) $(SRC_FILES) src/main.cpp -o bin/canny_app -lm

# 4. CANNY_RV: Cross-compiles pipeline for RISC-V
canny_rv:
	@mkdir -p bin_rv
	$(RV_CXX) $(RV_FLAGS) $(SRC_FILES) src/main.cpp -o bin_rv/canny_riscv $(RV_LIBS)

# 5. RUN: Executes RISC-V binary
run: canny_rv
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin_rv/canny_riscv 512 512 1 50 150

clean:
	rm -rf bin bin_rv

.PHONY: test test_rvv canny_host canny_rv run clean