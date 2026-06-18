HOST_CXX ?= g++
RV_CXX   ?= riscv64-unknown-elf-g++

OPT        ?= -O3
VEC_REPORT ?= -fopt-info-vec-all

GTEST_ROOT ?= /usr/local
HOST_FLAGS  = -I$(GTEST_ROOT)/include -Iinclude -Isrc $(OPT) $(VEC_REPORT)
HOST_LIBS   = -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -lpthread -lm

RV_FLAGS    = -Iinclude -Isrc -march=rv64gcv $(OPT) $(VEC_REPORT)
RV_LIBS     = -lm

SRC_FILES = $(filter-out src/main.cpp, $(wildcard src/*.cpp))

.PHONY: all clean host_tests rvv_tests test

all: host_tests rvv_tests

test: host_tests

host_tests: test_gaussian_host test_magnitude_host test_sobel_host test_direction_host

test_gaussian_host: tests/test_gaussian.cpp $(SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_magnitude_host: tests/test_magnitude.cpp $(SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_sobel_host: tests/test_sobel.cpp $(SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

test_direction_host: tests/test_direction.cpp $(SRC_FILES)
	$(HOST_CXX) $(HOST_FLAGS) $^ -o $@ $(HOST_LIBS)

rvv_tests: test_gaussian_rvv

test_gaussian_rvv: tests/test_gaussian_rvv.cpp $(SRC_FILES)
	$(RV_CXX) $(RV_FLAGS) $^ -o $@ $(RV_LIBS)

clean:
	rm -f test_gaussian_host test_magnitude_host test_sobel_host test_direction_host test_gaussian_rvv
