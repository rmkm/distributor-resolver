a.out: main.o distributor.o
	g++ -Wall -O2 -o a.out main.o distributor.o

main.o: main.cpp
	g++ -c main.cpp

distributor.o: distributor.cpp
	g++ -c distributor.cpp

clean:
	rm -f a.out main.o distributor.o
