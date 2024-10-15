.PHONY:all
all:DataBaseTest servertest server test
servertest:servertest.cc
	g++ -g $^ -o $@ -std=c++14 -ljsoncpp -lpthread -lsqlite3
server:../server/UDPserver_main.cc
	g++ -g $^ -o $@ -std=c++14 -ljsoncpp -lpthread -lsqlite3
test:test.cc
	g++ $^ -o $@ -std=c++14

DataBaseTest:DataBaseTest.cc
	g++ -g $^ -o $@ -std=c++14  -lsqlite3
.PHONY:clean
clean:
	rm -rf DataBaseTest ./data servertest server test ./log