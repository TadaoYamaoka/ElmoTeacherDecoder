CC = g++
CFLAGS = -std=c++11 -fno-exceptions -fno-rtti -Wextra -Ofast
INCLUDE = -I../hcp_decoder

sources = hcpe_to_hcp.cpp
common_sources =  bitboard.cpp hand.cpp init.cpp move.cpp mt64bit.cpp pieceScore.cpp position.cpp square.cpp
objects = $(addprefix obj/, $(sources:.cpp=.o))
common_objects = $(addprefix obj/, $(sources:.cpp=.o))

bin/hcpe_to_hcp: $(objects) $(common_objects)
	@[ -d bin ] || mkdir -p bin
	$(CC) -o $@ $^ $(CFLAGS)

obj/%.o: %.cpp
	@[ -d obj ] || mkdir -p obj
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

obj/%.o: ../hcp_decoder/%.cpp
	@[ -d obj ] || mkdir -p obj
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

all: bin/hcpe_to_hcp