# ==========================================
# Compilers Configuration
# ==========================================
HOST_CXX = g++
RV_CXX   = riscv64-unknown-elf-g++
# ==========================================
# Sources (auto-detected, no manual listing)
# ==========================================
TEST_SRCS = $(wildcard tests/*.cpp)
SRC_SRCS  = $(filter-out src/main.cpp, $(wildcard src/*.cpp))
# ==========================================
# Compilation Flags
# ==========================================
HOST_FLAGS = -I$(GTEST_ROOT)/include -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -lpthread
RV_FLAGS   = -march=rv64gcv -O2
# ==========================================
# Build Targets
# ==========================================
# 1. TEST: Compiles and runs each test file separately (avoids duplicate main conflict)
TEST_BINS = $(patsubst tests/%.cpp, bin/%, $(TEST_SRCS))

test: $(TEST_BINS)
	@echo "Running all tests..."
	@for t in $(TEST_BINS); do echo "--- $$t ---"; ./$$t; done

bin/%: tests/%.cpp $(SRC_SRCS)
	@mkdir -p bin
	$(HOST_CXX) $< $(SRC_SRCS) $(HOST_FLAGS) -o $@

# 2. CANNY_RV: Cross-compiles the pipeline for RISC-V target
canny_rv:
	@mkdir -p bin_rv
	$(RV_CXX) $(RV_FLAGS) src/main.cpp src/canny.cpp -o bin_rv/canny_riscv
# 3. RUN: Executes the compiled RISC-V binary on QEMU with VLEN=128
run: canny_rv
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin_rv/canny_riscv
# 4. CLEAN: Removes all generated binary and object files
clean:
	rm -rf bin bin_rv
