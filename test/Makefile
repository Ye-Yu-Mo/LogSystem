.PHONY: all clean run

# 平台检测：Mac 上 gtest/jsoncpp 路径与 Linux 不同
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    JSONCPP_SHIM := /tmp/jsoncpp_mac_shim
    JSONCPP_SETUP := mkdir -p $(JSONCPP_SHIM)/jsoncpp && ln -sfn /opt/homebrew/include/json $(JSONCPP_SHIM)/jsoncpp/json
    PLATFORM_FLAGS := -I$(JSONCPP_SHIM) -I/opt/homebrew/include -L/opt/homebrew/lib
else
    JSONCPP_SETUP :=
    PLATFORM_FLAGS :=
endif

CXX := g++
CXXFLAGS := -g -std=c++17 $(PLATFORM_FLAGS) -I.. -MMD -MP
GTEST_LIBS := -lgtest -lgtest_main -lpthread

# 每个 test_*.cc 编译成一个独立可执行文件
TESTS := test_level test_format test_buffer test_logger test_mpsc_queue

all: $(TESTS)

test_level: test_level.cc
	$(CXX) $(CXXFLAGS) $^ -o $@ $(GTEST_LIBS)

test_format: test_format.cc
	$(CXX) $(CXXFLAGS) $^ -o $@ $(GTEST_LIBS)

test_buffer: test_buffer.cc
	$(CXX) $(CXXFLAGS) $^ -o $@ $(GTEST_LIBS)

test_logger: test_logger.cc
	$(JSONCPP_SETUP)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(GTEST_LIBS) -lsqlite3

test_mpsc_queue: test_mpsc_queue.cc
	$(CXX) $(CXXFLAGS) $^ -o $@ $(GTEST_LIBS)

# 跑全部测试
run: all
	@for t in $(TESTS); do echo "=== $$t ==="; ./$$t || exit 1; done

clean:
	rm -rf $(TESTS) *.d *.dSYM ./test_log ./test_data

# 自动包含 -MMD 生成的头文件依赖，改 .hpp 会触发重编
-include $(wildcard *.d)
