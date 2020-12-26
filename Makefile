.PHONY: all clean

all: Diagram2.eps random-combiners.exe speed-v-epsilon.eps benchmark.exe \
	line-cl-hh24.eps amd-16.eps gcc-local-hh4.eps test-read-each-byte.exe test-read-each-byte.debug-exe no-collisions.exe no-collisions.debug-exe

RELEASE_FLAGS = -ggdb3 -O3 -march=native -Wall -Wextra -Wstrict-aliasing \
	-funroll-loops -fno-strict-aliasing -Wno-strict-overflow -DNDEBUG \
	-Wno-comment -Wno-ignored-attributes -Wno-constant-conversion \
	# -fsanitize=memory -fsanitize-blacklist=deny-list.txt #-stdlib=libc++

DEBUG_FLAGS = -ggdb3 -O0 -march=native -Wall -Wextra -Wstrict-aliasing \
	-funroll-loops -fno-strict-aliasing -Wno-strict-overflow -UNDEBUG \
	-Wno-comment -Wno-ignored-attributes -Wno-constant-conversion \

CXX_RELEASE_FLAGS = $(RELEASE_FLAGS) -std=c++14 #-stdlib=libc++
CXX_DEBUG_FLAGS = $(DEBUG_FLAGS) -std=c++14 #-stdlib=libc++
#DEBUGFLAGS = $(FLAGS) -ggdb3 -O0 -fno-unroll-loops -UNDEBUG
#CFLAGS = $(FLAGS) -std=gnu11
#CDEBUGFLAGS = $(DEBUGFLAGS) -std=gnu11
#CXXFLAGS = $(FLAGS) -std=c++11
#CXXDEBUGFLAGS = $(DEBUGFLAGS) -std=c++11
export

%.eps: %.dia Makefile
	dia -e $@ $<

umash/umash.o: umash/umash.c deny-list.txt Makefile
	$(CC) $(RELEASE_FLAGS) -o $@ $< -c

%.exe: %.cc $(shell find -name '*.hpp' ) deny-list.txt umash/umash.o Makefile
	$(CXX) $(CXX_RELEASE_FLAGS) -o $@ $< umash/umash.o

%.exe: %.cpp $(shell find -name '*.hpp' ) deny-list.txt umash/umash.o Makefile
	$(CXX) $(CXX_RELEASE_FLAGS) -o $@ $< umash/umash.o

%.debug-exe: %.cpp $(shell find -name '*.hpp' ) deny-list.txt umash/umash.o Makefile
	$(CXX) $(CXX_DEBUG_FLAGS) -o $@ $< umash/umash.o


amd-16.eps: plateau-008.txt points-example.txt plot.gnu Makefile
	gnuplot plot.gnu

line-cl-hh24.eps speed-v-epsilon.eps amd-cl-hh24.eps gcc-local-hh4.eps: plateau-008.txt points-example.txt plot.gnu amd-16.eps Makefile

clean: Makefile
	rm -f Diagram2.eps
	rm -f random-combiners.exe
	rm -f speed-v-epsilon.eps
	rm -f gcc-local-hh4.eps
	rm -f amd-16.eps
	rm -f amd-24.eps
	rm -f amd-cl-hh24.eps
	rm -f benchmark.exe
	rm -f line-cl-hh24.eps
	rm -f umash/umash.o
	rm -f test-read-each-byte.exe
	rm -f test-read-each-byte.debug-exe
	rm -f no-collisions.exe
	rm -f no-collisions.debug-exe
