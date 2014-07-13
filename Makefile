
all: rr_a name_pkt

rr_a: rr_a.cpp name_pkt.hpp
	c++ -g -Wall -DDEBUG -o ./rr_a ./rr_a.cpp -Wno-unused-variable

name_pkt: name_pkt.cpp name_pkt.hpp
	# macosx
	c++ -g -Wall -o ./name_pkt ./name_pkt.cpp -lresolv
	# FreeBSD, Linux
	#c++ -g -Wall -o ./name_pkt ./name_pkt.cpp

clean:
	rm -rf *.dSYM name_pkt rr_a
