src = src
lib =
build_dir = build
target = CASE_Cache_parallel_LRU
objects = main.o
cc = g++ -std=c++11 -msse2 -mbmi -lm -Wall -fmessage-length=0 -pthread -O3

$(target): $(objects)
	$(cc) -o $(target) $(objects)

main.o: main.cpp big_lru.h small_lru.h Safe_Queue.h
	$(cc) -c main.cpp -o main.o

.PHONY: clean
clean:
	rm -rf $(objects)  $(target)
