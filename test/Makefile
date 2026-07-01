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

TESTS := test_level test_format test_logger test_mpsc_queue test_logquery

all: $(TESTS)

test_level: test_level.cc
	$(CXX) $(CXXFLAGS) $< -o $@ $(GTEST_LIBS)

test_format: test_format.cc
	$(CXX) $(CXXFLAGS) $< -o $@ $(GTEST_LIBS)

test_logger: test_logger.cc
	$(JSONCPP_SETUP)
	$(CXX) $(CXXFLAGS) $< -o $@ $(GTEST_LIBS) -lsqlite3

test_mpsc_queue: test_mpsc_queue.cc
	$(CXX) $(CXXFLAGS) $< -o $@ $(GTEST_LIBS)

test_logquery: test_logquery.cc
	$(JSONCPP_SETUP)
	$(CXX) $(CXXFLAGS) $< -o $@ $(GTEST_LIBS) -lsqlite3 -ljsoncpp

run: all
	@for t in $(TESTS); do echo "=== $$t ==="; ./$$t || exit 1; done

clean:
	rm -rf $(TESTS) *.d *.dSYM ./test_log ./test_data

-include $(wildcard *.d)
