.PHONY: all clean

all: Diagram2.eps random-combiners.exe speed-v-epsilon.eps

RELEASE_FLAGS = -ggdb3 -O3 -march=native -Wall -Wextra -Wstrict-aliasing \
	-funroll-loops -fno-strict-aliasing -Wno-strict-overflow -DNDEBUG \
	-Wno-comment -Wno-ignored-attributes
#DEBUGFLAGS = $(FLAGS) -ggdb3 -O0 -fno-unroll-loops -UNDEBUG
#CFLAGS = $(FLAGS) -std=gnu11
#CDEBUGFLAGS = $(DEBUGFLAGS) -std=gnu11
#CXXFLAGS = $(FLAGS) -std=c++11
#CXXDEBUGFLAGS = $(DEBUGFLAGS) -std=c++11
export

%.eps: %.dia Makefile
	dia -e $@ $<

%.exe: %.cc $(shell find -name '*.hpp' ) Makefile
	$(CXX) $(RELEASE_FLAGS) -o $@ $<

speed-v-epsilon.eps: points-example.txt plot.gnu Makefile
	gnuplot plot.gnu

clean: Makefile
	rm Diagram2.eps
