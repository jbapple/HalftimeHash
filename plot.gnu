set terminal postscript eps enhanced color size 12cm,6.4cm fontfile "/usr/share/texmf/fonts/type1/public/lm/lmr17.pfb" "LMRoman17,17"
set title "SMhasher speed tests (higher is better)"
set ylabel "bytes per cycle"
set key noautotitle
set xtics rotate by -60
set style fill solid noborder
set boxwidth 0.75
set rmargin 5
set style data histogram
set style histogram rowstacked
unset logscale y
unset logscale x
set grid
unset key
set output 'smhasher-speed.eps'
set yrange[8:*]
set style fill
plot 'smhasher-speed.txt' using 4:xtic(1), '' using 3, '' using 2
unset style

set terminal postscript eps enhanced color size 9.6cm,6.4cm fontfile "/usr/share/texmf/fonts/type1/public/lm/lmr17.pfb" "LMRoman17,17"
set output 'speed-v-epsilon.eps'
set ylabel 'bytes per nanosecond'
set xlabel 'output entropy'
set title "Output entropy vs. B/ns (upper right is better)"
unset key
set logscale x 2
unset logscale y
unset grid
set xrange[2**20:2**160]
set yrange[10:70]
unset offsets
set format x '%L'
plot 'points-example.txt' using (1/column(2)):1:3 with labels point pt 7 offset char -1,1, '' using (1/column(5)):4:6 with labels point pt 7 offset char 2,-1

set terminal postscript eps enhanced color size 12cm,6.4cm fontfile "/usr/share/texmf/fonts/type1/public/lm/lmr17.pfb" "LMRoman17,17"
set output 'speed-v-epsilon-amd.eps'
set ylabel 'bytes per nanosecond'
set xlabel 'output entropy'
set title "Output entropy vs. B/ns (upper right is better)"
unset key
set logscale x 2
unset logscale y
set grid
set xrange[2**20:2**160]
set yrange[5:50]
unset offsets
set format x '%L'
plot 'c5a.large-clang-11-925ea6f.txt'  using (1/column(2)):1:3 with labels point pt 7 offset char -1,1, '' using (1/column(5)):4:6 with labels point pt 7 offset char 2,-1

set terminal postscript eps enhanced color size 6.4cm,6.4cm fontfile "/usr/share/texmf/fonts/type1/public/lm/lmr17.pfb" "LMRoman17,17"
#set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
unset title
unset offsets
set xlabel 'input length in bytes'
set ylabel 'bytes per nanosecond'
set key top left
set key autotitle columnheader
unset logscale y
set logscale x
unset format y
set format x '10^{%L}'
set grid
set output 'amd-16.eps'
set yrange[*:55]
set xrange[*:*]
plot './c5a.large-clang-11-604af20.txt' using 1:3 with lines, '' using 1:4 pt 6

set terminal postscript eps enhanced color size 6.4cm,6.4cm fontfile "/usr/share/texmf/fonts/type1/public/lm/lmr17.pfb" "LMRoman17,17"
#set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
unset title
unset offsets
set xlabel 'input length in bytes'
unset ylabel
set key top left
set key autotitle columnheader
unset logscale y
set logscale x
unset format y
set format x '10^{%L}'
set grid
set output 'amd-24.eps'
set yrange[*:55]
set xrange[*:*]
plot './c5a.large-clang-11-22bd15d.txt' using 1:4 with lines, './c5a.large-clang-11-22bd15d.txt' using 1:8 pt 6
#plot for [i=3:3] './c5a.large-clang-11-8c4af1f.txt' using 1:i with lines,  for [i=7:7] './c5a.large-clang-11-8c4af1f.txt' using 1:i pt 6

# set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
# set title "Input size vs. GB/s (higher is better)"
# unset offsets
# set xlabel 'input length in bytes'
# set ylabel 'billion bytes per second'
# set key top left
# set key autotitle columnheader
# unset logscale y
# set logscale x
# unset format y
# set format x '10^{%L}'
# set grid
# set output 'line-cl-hh24.eps'
# set yrange[*:90]
# set xrange[*:*]
# plot './umash-001.txt' using 1:2 pt 6, '' using 1:3 with lines lw 10, '' using 1:4 with lines

set terminal postscript eps enhanced color size 6.4cm,6.4cm fontfile "/usr/share/texmf/fonts/type1/public/lm/lmr17.pfb" "LMRoman17,17"
#set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
set title 'i7-7800x speed (higher is better)'
unset offsets
set xlabel 'input length in bytes'
set ylabel 'bytes per nanosecond'
set key top left
set key autotitle columnheader
unset logscale y
set logscale x
unset format y
set format x '10^{%L}'
set grid
set output 'line-cl-hh24.eps'
set yrange[*:90]
set xrange[*:*]
plot './adu-full-001.txt' using 1:4 pt 6, '' using 1:11 with lines lw 10, '' using 1:13 with lines

set terminal postscript eps enhanced color size 6.4cm,6.4cm fontfile "/usr/share/texmf/fonts/type1/public/lm/lmr17.pfb" "LMRoman17,17"
#set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
set title 'i7-7800x speed (higher is better)'
unset offsets
set xlabel 'input length in bytes'
set ylabel 'bytes per nanosecond'
set key top left
set key autotitle columnheader
unset logscale y
set logscale x
unset format y
set format x '10^{%L}'
set grid
set output 'line-cl-hh24-floor.eps'
set yrange[*:90]
set xrange[1000:*]
plot './adu-full-001.txt' using 1:4 pt 6, '' using 1:11 with lines lw 10, '' using 1:13 with lines

set terminal postscript eps enhanced color size 6.4cm,6.4cm fontfile "/usr/share/texmf/fonts/type1/public/lm/lmr17.pfb" "LMRoman17,17"
#set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
set title '7R32 speed (higher is better)'
unset offsets
set xlabel 'input length in bytes'
unset ylabel
set key top left
set key autotitle columnheader
unset logscale y
set logscale x
unset format y
set format x '10^{%L}'
set grid
set output 'amd-cl-hh24.eps'
set yrange[*:90]
set xrange[*:*]
plot './c5a.large-clang-11-22bd15d.txt' using 1:8 pt 6, '' using 1:11 with lines lw 10, '' using 1:13 with lines
#plot './c5a.large-clang-11-8c4af1f.txt' using 1:7 pt 6, './c5a.large-gcc-10-8c4af1f.txt' using 1:10 with lines lw 10, './c5a.large-clang-11-8c4af1f.txt' using 1:12 with lines

set terminal postscript eps enhanced color size 6.4cm,6.4cm fontfile "/usr/share/texmf/fonts/type1/public/lm/lmr17.pfb" "LMRoman17,17"
#set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
set title '7R32 speed (higher is better)'
unset offsets
set xlabel 'input length in bytes'
unset ylabel
set key top left
set key autotitle columnheader
unset logscale y
set logscale x
unset format y
set format x '10^{%L}'
set grid
set output 'amd-cl-hh24-floor.eps'
set yrange[*:90]
set xrange[1000:*]
plot './c5a.large-clang-11-22bd15d.txt' using 1:8 pt 6, '' using 1:11 with lines lw 10, '' using 1:13 with lines
#plot './c5a.large-clang-11-8c4af1f.txt' using 1:7 pt 6, './c5a.large-gcc-10-8c4af1f.txt' using 1:10 with lines lw 10, './c5a.large-clang-11-8c4af1f.txt' using 1:12 with lines

set terminal postscript eps enhanced color size 12cm,6.4cm fontfile "/usr/share/texmf/fonts/type1/public/lm/lmr17.pfb" "LMRoman17,17"
set title "Input size vs. B/ns (higher is better)"
unset offsets
set xlabel 'input length in bytes'
set ylabel 'bytes per nanosecond'
set key top left
set key autotitle columnheader
unset logscale y
set logscale x
unset format y
set format x '10^{%L}'
set grid
set output 'clang-local-hh4.eps'
set yrange[*:90]
set xrange[*:*]
plot for [i=3:6] './simpler-k2-multipliers-002.txt' using 1:i with lines

