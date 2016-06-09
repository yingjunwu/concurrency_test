
all: test_performance

test_performance: test_performance.o
	g++ test_performance.o -o test_performance -pthread --std=c++11 -O3 -ljemalloc

test_performance.o: libcuckoo_performance.cpp
	g++ libcuckoo_performance.cpp -c -pthread --std=c++11 -o test_performance.o -O3 -ljemalloc

clean:
	rm -f *.o *.log test_performance
	
