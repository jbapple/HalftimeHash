set terminal postscript eps enhanced color size 5,6 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
set output 'speed-v-epsilon.eps'
set ylabel 'bytes per nanosecond'
set xlabel 'output entropy'
set title "Speed vs. output entropy (upper right is better)"
unset key
set logscale x 2
unset logscale y
set grid
set xrange[2**20:2**160]
set yrange[0:80]
unset offsets
set format x '%L'
plot 'points-example.txt' using (1/column(2)):1:3 with labels point pt 7 offset char 0,1

set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
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
set yrange[*:65]
set xrange[*:*]
plot for [i=2:2:4] './c5a.large-gcc-10-8c4af1f.txt' using 1:i with lines,  for [i=6:9:4] './c5a.large-clang-11-8c4af1f.txt' using 1:i pt 6

set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
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
set yrange[*:65]
set xrange[*:*]
plot for [i=3:3] './c5a.large-clang-11-8c4af1f.txt' using 1:i with lines,  for [i=7:7] './c5a.large-clang-11-8c4af1f.txt' using 1:i pt 6

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

set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
set title 'i7-7800x'
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
plot './umash-001.txt' using 1:2 pt 6, '' using 1:3 with lines lw 10, '' using 1:4 with lines

set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
set title '7R32'
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
plot './c5a.large-clang-11-8c4af1f.txt' using 1:7 pt 6, './c5a.large-gcc-10-8c4af1f.txt' using 1:10 with lines lw 10, './c5a.large-clang-11-8c4af1f.txt' using 1:12 with lines

set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
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
set output 'gcc-local-hh4.eps'
set yrange[*:90]
set xrange[*:*]
plot for [i=2:5] './gcc-hh4.txt' using 1:i with lines
