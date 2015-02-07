.PHONY: parsim

parsim:
	mkdir -p bin
	g++ -std=c++11 -o bin/parsim src/parsim.cpp -lpthread

test:
	bin/parsim -s 0 10

clean:
	rm -rf bin