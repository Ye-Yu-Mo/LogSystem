.PHONY: all clean

# 平台检测：Mac 上 jsoncpp 头文件路径与 Linux 不同
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    JSONCPP_SHIM := /tmp/jsoncpp_mac_shim
    JSONCPP_SETUP := mkdir -p $(JSONCPP_SHIM)/jsoncpp && ln -sfn /opt/homebrew/include/json $(JSONCPP_SHIM)/jsoncpp/json
    JSONCPP_FLAGS := -I$(JSONCPP_SHIM) -I/opt/homebrew/include -L/opt/homebrew/lib
else
    JSONCPP_SETUP :=
    JSONCPP_FLAGS :=
endif

CXX := g++
CXXFLAGS := -g -std=c++14 -MMD -MP

all: DataBaseTest servertest server test

servertest: servertest.cc
	$(JSONCPP_SETUP)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(JSONCPP_FLAGS) -ljsoncpp -lpthread -lsqlite3

server: ../server/ServerMain.cc ../config/INIReader.cpp ../config/ini.c
	$(JSONCPP_SETUP)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(JSONCPP_FLAGS) -ljsoncpp -lpthread -lsqlite3

test: test.cc
	$(CXX) $(CXXFLAGS) $^ -o $@

DataBaseTest: DataBaseTest.cc
	$(CXX) $(CXXFLAGS) $^ -o $@ -lsqlite3

clean:
	rm -rf DataBaseTest servertest server test ./log *.d *.dSYM

# 自动头文件依赖：改 .hpp 触发重编
-include $(wildcard *.d)
