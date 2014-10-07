server.o: 
	g++ -Wall -Wno-sign-compare  -o  server  server.cpp
clean:
	rm -rf server
