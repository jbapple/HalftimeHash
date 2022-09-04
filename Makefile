.PHONY: all clean

all: Diagram2.eps random-combiners.exe speed-v-epsilon.eps benchmark.exe \
	line-cl-hh24.eps amd-16.eps clang-local-hh4.eps test-read-each-byte.exe \
	test-read-each-byte.debug-exe no-collisions.exe no-collisions.debug-exe \
	smhasher-speed.eps test-bytes-needed.exe example.exe yes-badger.eps \
	no-badger.eps speed-v-epsilon-amd.eps arxiv.zip smhasher-speed.pdf \
	speed-v-epsilon-amd.pdf line-cl-hh24.pdf amd-cl-hh24.pdf easychair.zip \
	tree.eps tree-1.eps tree-2.eps tree-3.eps check-consistency.exe

RELEASE_FLAGS = -ggdb3 -O3 -march=icelake-server -Wall -Wextra -Wstrict-aliasing \
	-funroll-loops -fno-strict-aliasing -Wno-strict-overflow -DNDEBUG \
	-Wno-comment -Wno-ignored-attributes -Wno-constant-conversion \
	-Wno-unused-lambda-capture
	# -fsanitize=memory -fsanitize-blacklist=deny-list.txt #-stdlib=libc++

DEBUG_FLAGS = -ggdb3 -O0 -march=icelake-server -Wall -Wextra -Wstrict-aliasing \
	-funroll-loops -fno-strict-aliasing -Wno-strict-overflow -UNDEBUG \
	-Wno-comment -Wno-ignored-attributes -Wno-constant-conversion \
	-Wno-unused-lambda-capture

CXX_RELEASE_FLAGS = $(RELEASE_FLAGS) -std=c++14 #-DIACA -I/home/jbapple/progs/iaca/iaca-lin64/ #-stdlib=libc++
CXX_DEBUG_FLAGS = $(DEBUG_FLAGS) -std=c++14 #-stdlib=libc++
#DEBUGFLAGS = $(FLAGS) -ggdb3 -O0 -fno-unroll-loops -UNDEBUG
#CFLAGS = $(FLAGS) -std=gnu11
#CDEBUGFLAGS = $(DEBUGFLAGS) -std=gnu11
#CXXFLAGS = $(FLAGS) -std=c++11
#CXXDEBUGFLAGS = $(DEBUGFLAGS) -std=c++11
export

%.eps: %.dia Makefile
	dia -e $@ $<

%.pdf: %.eps Makefile
	epstopdf $<

umash/umash.o: umash/umash.c Makefile
	$(CC) $(RELEASE_FLAGS) -o $@ $< -c

%.exe: %.cc $(shell find -name '*.hpp' ) umash/umash.o Makefile
	$(CXX) $(CXX_RELEASE_FLAGS) -o $@ $< umash/umash.o

%.exe: %.cpp $(shell find -name '*.hpp' ) umash/umash.o Makefile
	$(CXX) $(CXX_RELEASE_FLAGS) -o $@ $< umash/umash.o

%.debug-exe: %.cpp $(shell find -name '*.hpp' ) umash/umash.o Makefile
	$(CXX) $(CXX_DEBUG_FLAGS) -o $@ $< umash/umash.o

amd-16.eps: plateau-008.txt points-example.txt smhasher-speed.txt c5a.large-clang-11-925ea6f.txt plot.gnu Makefile
	gnuplot plot.gnu

smhasher-speed.eps line-cl-hh24.eps speed-v-epsilon.eps amd-cl-hh24.eps clang-local-hh4.eps speed-v-epsilon-amd.eps: plateau-008.txt points-example.txt plot.gnu amd-16.eps Makefile

smhasher-speed.pdf line-cl-hh24.pdf speed-v-epsilon.pdf amd-cl-hh24.pdf clang-local-hh4.pdf speed-v-epsilon-amd.pdf: plateau-008.txt points-example.txt plot.gnu amd-16.eps Makefile

arxiv.zip: smhasher-speed.pdf speed-v-epsilon.pdf line-cl-hh24.pdf amd-cl-hh24.pdf halftime-hash.tex halftime-hash.bbl Makefile
	rm -f arxiv.zip
	zip arxiv $^
	zip -d arxiv.zip Makefile

easychair.zip: smhasher-speed.eps speed-v-epsilon.eps line-cl-hh24.eps amd-cl-hh24.eps halftime-hash.tex halftime-hash.bib Makefile
	rm -f easychair.zip
	zip easychair $^
	zip -d easychair.zip Makefile


clean: Makefile
	rm -f smhasher-speed.pdf line-cl-hh24.pdf speed-v-epsilon.pdf amd-cl-hh24.pdf clang-local-hh4.pdf speed-v-epsilon-amd.pdf
	rm -f Diagram2.eps
	rm -f random-combiners.exe
	rm -f speed-v-epsilon.eps
	rm -f clang-local-hh4.eps
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
	rm -f smhasher-speed.eps
	rm -f test-byes-needed.exe
