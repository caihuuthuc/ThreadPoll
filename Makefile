.PHONY: all clean

all:
  g++ -std=c+=17 examples/accumulation.cpp -I. -o accumulation.o
  g++ -std=c++17 main.cpp -o main.o

clean:
  rm -f *.o
