
all: rr_a

rr_a: rr_a.cpp name_pkt.hpp
	c++ -g -Wall -DDEBUG -std=c++11 -o ./rr_a ./rr_a.cpp -Wno-unused-variable

pkt_buffer: pkt_buffer.cpp pkt_buffer.hpp
	c++ -g -Wall -o ./pkt_buffer ./pkt_buffer.cpp

name_pkt: name_pkt.cpp name_pkt.hpp
	# macosx
	c++ -g -Wall -o ./name_pkt ./name_pkt.cpp -lresolv
	# FreeBSD, Linux
	#c++ -g -Wall -o ./name_pkt ./name_pkt.cpp

clean:
	rm -rf *.dSYM name_pkt rr_a pkt_buffer
