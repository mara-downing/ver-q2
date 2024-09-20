all: main.o nnet_q.o symformula.o leaflist.o leaf.o instruction.o abstract.o bigsizet.o helpers.o provero_impl.o
	g++ -o main main.o nnet_q.o symformula.o leaflist.o leaf.o instruction.o abstract.o bigsizet.o helpers.o -O0 -ggdb -Wall -lz3
	g++ -o provero_impl provero_impl.o nnet_q.o symformula.o leaflist.o leaf.o instruction.o abstract.o bigsizet.o helpers.o -O0 -ggdb -Wall -lz3

main.o: main.cpp nnet_q.cpp nnet_q.hpp symformula.cpp symformula.hpp leaflist.cpp leaflist.hpp leaf.cpp leaf.hpp instruction.cpp instruction.hpp abstract.cpp abstract.hpp bigsizet.cpp bigsizet.hpp helpers.cpp helpers.hpp
	g++ -c main.cpp nnet_q.cpp symformula.cpp leaflist.cpp leaf.cpp instruction.cpp abstract.cpp bigsizet.cpp helpers.cpp -O0 -ggdb -Wall -lz3

provero_impl.o: provero_impl.cpp nnet_q.cpp nnet_q.hpp symformula.cpp symformula.hpp leaflist.cpp leaflist.hpp leaf.cpp leaf.hpp instruction.cpp instruction.hpp abstract.cpp abstract.hpp bigsizet.cpp bigsizet.hpp helpers.cpp helpers.hpp
	g++ -c provero_impl.cpp nnet_q.cpp symformula.cpp leaflist.cpp leaf.cpp instruction.cpp abstract.cpp bigsizet.cpp helpers.cpp -O0 -ggdb -Wall -lz3

clean:
	rm *.o main
