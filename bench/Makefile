.PHONY: all clean

CXX := g++
CXXFLAGS := -g -std=c++14 -MMD -MP

all: bench_test

bench_test: bench.cc
	$(CXX) $(CXXFLAGS) $^ -o $@ -lpthread

clean:
	rm -rf bench_test ./log *.d *.dSYM

# 自动头文件依赖：改 .hpp 触发重编
-include $(wildcard *.d)
