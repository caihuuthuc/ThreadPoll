.PHONY: all clean

all:
  g++ -std=c+=17 examples/accumulation.cpp thread_pool.hpp -o accumulation.o
  g++ -std=c++17 main.cpp -o main.o

clean:
  rm -f *.o
