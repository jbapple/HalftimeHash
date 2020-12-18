.PHONY: all clean

all: Diagram2.eps random-combiners.exe speed-v-epsilon.eps benchmark.exe line-cl-hh24.eps

RELEASE_FLAGS = -ggdb3 -O3 -march=native -Wall -Wextra -Wstrict-aliasing \
	-funroll-loops -fno-strict-aliasing -Wno-strict-overflow -DNDEBUG \
	-Wno-comment -Wno-ignored-attributes

#CXX_RELEASE-std=gnu++17
#DEBUGFLAGS = $(FLAGS) -ggdb3 -O0 -fno-unroll-loops -UNDEBUG
#CFLAGS = $(FLAGS) -std=gnu11
#CDEBUGFLAGS = $(DEBUGFLAGS) -std=gnu11
#CXXFLAGS = $(FLAGS) -std=c++11
#CXXDEBUGFLAGS = $(DEBUGFLAGS) -std=c++11
export

%.eps: %.dia Makefile
	dia -e $@ $<

umash/umash.o: umash/umash.c Makefile
	$(CC) $(RELEASE_FLAGS) -o $@ $< -c

%.exe: %.cc $(shell find -name '*.hpp' ) umash/umash.o Makefile
	$(CXX) $(RELEASE_FLAGS) -o $@ $< umash/umash.o

%.exe: %.cpp $(shell find -name '*.hpp' ) umash/umash.o Makefile
	$(CXX) $(RELEASE_FLAGS) -o $@ $< umash/umash.o

line-cl-hh24.eps speed-v-epsilon.eps: plateau-008.txt points-example.txt plot.gnu Makefile
	gnuplot plot.gnu

clean: Makefile
	rm -f Diagram2.eps
	rm -f random-combiners.exe
	rm -f speed-v-epsilon.eps
	rm -f benchmark.exe
	rm -f line-cl-hh24.eps
	rm -f umash/umash.o
