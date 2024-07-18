compile:
	g++ client.cpp -std=c++11 -o client.o -lboost_system -lboost_filesystem
	g++ server.cpp -std=c++11 -o server.o -lcrypto
clean:
	rm client.o
	rm server.o