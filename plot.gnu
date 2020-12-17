set terminal postscript eps enhanced color size 5,5 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
set output 'speed-v-epsilon.eps'
set xlabel 'nanoseconds per byte'
set ylabel 'collision probability'
set title "Speed vs. collision (lower left is better)"
unset key
set logscale y 2
unset logscale x
set grid
set yrange[1e-50:1e-10]
set xrange[0.0:0.08]
unset offsets
set xrange[0.0:0.08]
set format y '2^{%L}'
plot 'points-example.txt' using 1:2:3 with labels point pt 7 offset char 0,1

set terminal postscript eps enhanced color size 5,7 fontfile "/usr/share/texlive/texmf-dist/fonts/type1/public/libertine/LinLibertineOB.pfb" "LinLibertineOB,29"
set output 'line-cl-hh24.eps'
set title "Input size vs. GB/s (higher is better)"
unset offsets
set xlabel 'input length in bytes'
set ylabel 'billion bytes per second'
set key top left
set key autotitle columnheader
unset logscale y
set logscale x
unset format y
set format x '10^{%L}'
set grid
set yrange[*:*]
set xrange[*:*]
plot './plateau-008.txt' using 1:2 with lines, '' using 1:3