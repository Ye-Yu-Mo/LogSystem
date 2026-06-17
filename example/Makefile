.PHONY: all clean

# 平台检测：Mac 上 jsoncpp 头文件路径与 Linux 不同
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    # Homebrew jsoncpp: headers at /opt/homebrew/include/json，但代码 include 用 jsoncpp/json/
    # 建一个 shim 目录让 <jsoncpp/json/json.h> 能找到
    JSONCPP_SHIM := /tmp/jsoncpp_mac_shim
    JSONCPP_SETUP := mkdir -p $(JSONCPP_SHIM)/jsoncpp && ln -sfn /opt/homebrew/include/json $(JSONCPP_SHIM)/jsoncpp/json
    JSONCPP_FLAGS := -I$(JSONCPP_SHIM) -I/opt/homebrew/include -L/opt/homebrew/lib
else
    JSONCPP_SETUP :=
    JSONCPP_FLAGS :=
endif

all: DataBaseTest servertest server test

servertest: servertest.cc
	$(JSONCPP_SETUP)
	g++ -g $^ -o $@ -std=c++14 $(JSONCPP_FLAGS) -ljsoncpp -lpthread -lsqlite3

server: ../server/ServerMain.cc ../config/INIReader.cpp ../config/ini.c
	$(JSONCPP_SETUP)
	g++ -g $^ -o $@ -std=c++14 $(JSONCPP_FLAGS) -ljsoncpp -lpthread -lsqlite3

test: test.cc
	g++ $^ -o $@ -std=c++14

DataBaseTest: DataBaseTest.cc
	g++ -g $^ -o $@ -std=c++14 -lsqlite3

clean:
	rm -rf DataBaseTest servertest server test ./log
