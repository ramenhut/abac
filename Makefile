abac-test:
	g++ test.cpp bitstream.cpp cabac.cpp memory.cpp -O3 -o abac-test
debug:
	g++ test.cpp bitstream.cpp cabac.cpp memory.cpp -DDEBUG -Wall -g -o abac-test
clean:
	rm -f abac-test
