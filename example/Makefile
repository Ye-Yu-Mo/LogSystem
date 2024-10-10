# 编译器与编译选项
CXX := g++
CXXFLAGS := -std=c++14 -lpthread -Wall -g

# 源文件和目标文件
SERVER_SRC := ../server/server_main.cc
SERVER_TEST_SRC := servertest.cc
SERVER_BIN := server
SERVER_TEST_BIN := servertest

# 默认目标
.PHONY: all
all: $(SERVER_BIN) $(SERVER_TEST_BIN)

# 编译 server
$(SERVER_BIN): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

# 编译 servertest
$(SERVER_TEST_BIN): $(SERVER_TEST_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

# 清理生成的二进制文件
.PHONY: clean
clean:
	rm -rf $(SERVER_BIN) $(SERVER_TEST_BIN) log


#test:test.cc
#	g++ -std=c++11 $^ -o $@ -lpthread
#.PHONY: clean
#clean:
#	rm -rf test servertest ./log