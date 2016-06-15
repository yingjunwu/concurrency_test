
all: test_performance

#test_performance: test_performance.o
#	g++ test_performance.o -o test_performance -pthread --std=c++11 -O3 -ljemalloc

test_performance: libcuckoo_performance.cpp cuckoo_map.cpp
	g++ libcuckoo_performance.cpp cuckoo_map.cpp -pthread -lpthread --std=c++11 -o test_performance -O3 -ljemalloc -lcds

clean:
	rm -f *.o *.log test_performance
	
