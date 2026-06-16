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

RV_FLAGS   = -Iinclude -march=rv64gcv -O3
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

# 2. CANNY_RV: Cross-compiles the pipeline for RISC-V target
canny_rv:
	@mkdir -p bin_rv
	$(RV_CXX) $(RV_FLAGS) $(SRC_FILES) src/main.cpp -o bin_rv/canny_riscv $(RV_LIBS)

# 3. RUN: Executes the compiled RISC-V binary on QEMU
run: canny_rv
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin_rv/canny_riscv 512 512 1 50 150

# 4. CLEAN: Removes generated binaries
clean:
	rm -rf bin bin_rv

.PHONY: test canny_rv run clean