HOST_CXX = g++
RV_CXX   = riscv64-unknown-elf-clang++

GTEST_ROOT = $(HOME)/gtest-install
HOST_FLAGS = -std=c++17 -I$(GTEST_ROOT)/include -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -lpthread

RV_FLAGS = -march=rv64gcv -O2 -target riscv64-unknown-elf --gcc-toolchain=$(HOME)/riscv -static

test:
	@mkdir -p bin
	$(HOST_CXX) $(HOST_FLAGS) -Isrc tests/test_magnitude.cpp src/magnitude.cpp -o bin/unit_tests
	./bin/unit_tests

clean:
	rm -rf bin bin_rv
