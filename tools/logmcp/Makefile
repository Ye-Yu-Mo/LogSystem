.PHONY: all clean

UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    JSONCPP_SHIM := /tmp/jsoncpp_mac_shim
    JSONCPP_FLAGS := -I$(JSONCPP_SHIM) -I/opt/homebrew/include -L/opt/homebrew/lib
else
    JSONCPP_FLAGS :=
endif

CXX := g++
CXXFLAGS := -g -std=c++17 -I.. $(JSONCPP_FLAGS) -MMD -MP

all: logmcp

logmcp: main.cc ../LogQuery.hpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(JSONCPP_FLAGS) -lsqlite3 -ljsoncpp

clean:
	rm -rf logmcp *.d *.dSYM

-include $(wildcard *.d)
