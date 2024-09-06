test:test.cc
	g++ -g -std=c++11 $^ -o $@
.PHONY: clean
clean:
	rm -f test