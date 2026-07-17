.PHONY: all clean test

UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    JSONCPP_SHIM := /tmp/jsoncpp_mac_shim
    JSONCPP_FLAGS := -I$(JSONCPP_SHIM) -I/opt/homebrew/include -L/opt/homebrew/lib
else
    JSONCPP_FLAGS :=
endif

CXX := g++
CXXFLAGS := -g -std=c++17 -I.. $(JSONCPP_FLAGS) -MMD -MP

all: logweb

logweb: main.cc ../LogQuery.hpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(JSONCPP_FLAGS) -lsqlite3 -ljsoncpp

test: logweb
	bash test_logweb.sh

clean:
	rm -rf logweb *.d *.dSYM

-include $(wildcard *.d)
