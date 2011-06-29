set terminal postscript eps
set output 
set noclip points
set clip one
set noclip two
set border
set boxwidth
set dummy x,y
set format x "%g"
set format y "%g"
set format z "%g"
set nogrid
set key
set nolabel
set noarrow
set nologscale
set offsets 0, 0, 0, 0
set nopolar
set angles radians
set noparametric
set view 60, 30, 1, 1
set samples 100, 100
set isosamples 10, 10
set surface
set nocontour
set clabel
set nohidden3d
set cntrparam order 4
set cntrparam linear
set cntrparam levels auto 5
#set cntrparam points 5
set size 1,1
set data style points
set function style lines
set noxzeroaxis
set noyzeroaxis
set tics in
set ticslevel 0.5
set ztics
set title "" 0,0
set notime
set rrange [-0 : 10]
set trange [-5 : 5]
set urange [-5 : 5]
set vrange [-5 : 5]
set xlabel "Messages - NAME" 0,0
set xrange [0 : 1]
set ylabel "% Misclassification (logit scale)" 0,0
set zlabel "" 0,0
set zrange [-10 : 10]
set autoscale r
set autoscale t
set noautoscale   
set autoscale z
set zero 1e-08
set autoscale x
#set autoscale y
set yrange [-4 : .5]
#set yrange [-4 : 2]
#set xtics ("0.00001" -7.000,"0.0001" -6.000,"0.001" -5.000,"0.01" -4.000,"0.10" -3.000,"1.00" -1.996,"10.00" -0.954,"50.00" 0.000,"90.00" 0.954,"99.00" 1.996,"99.90" 3.000,"99.99" 4.000,"99.999" 5.000,"99.9999" 6.000,"99.99999" 7.000) rotate by 45  offset -3.0,-2.0 
set ytics ("0.00001" 7.000,"0.0001" 6.000,"0.001" 5.000,"0.01" 4.000,"0.10" 3.000,"1.00" 1.996,"10.00" 0.954,"50.00" 0.000,"90.00" -0.954,"99.00" -1.996,"99.90" -3.000,"99.99" -4.000,"99.999" -5.000,"99.9999" -6.000,"99.99999" -7.000)
#set xtics 
#set ytics
set boxwidth 1.5 absolute
set style fill  solid 0.25 noborder

set pointsize 0.9
# tango color palette: http://tango.freedesktop.org/Tango_Icon_Theme_Guidelines#Color_Palette
set style line 1  linetype 3 lc rgb "#0909e9" linewidth 1
set style line 2  linetype 3 lc rgb "#e90909" linewidth 1

set style line 3  linetype 1 lc rgb "#ef2929" linewidth 3         pt 1
set style line 4  linetype 1 lc rgb "#ad7fa8" linewidth 3         pt 2
set style line 5  linetype 1 lc rgb "#729fcf" linewidth 3         pt 3 
set style line 6  linetype 1 lc rgb "#73d216" linewidth 3         pt 4 
set style line 7  linetype 1 lc rgb "#c17d11" linewidth 3         pt 5 
set style line 8  linetype 1 lc rgb "#f57900" linewidth 3         pt 6 
set style line 9  linetype 1 lc rgb "#edd400" linewidth 3         pt 7 
set style line 10 linetype 1 lc rgb "#5c3566" linewidth 3         pt 8 
set style line 11 linetype 1 lc rgb "#204a87" linewidth 3         pt 9 
set style line 12 linetype 1 lc rgb "#4e9a06" linewidth 3         pt 10 
set style line 13 linetype 1 lc rgb "#8f5902" linewidth 3         pt 11 
set style line 14 linetype 1 lc rgb "#ce5c00" linewidth 3         pt 12 





plot (1/0) title "" \
     "tmp/NAME.all.txt"   using 1:3 title "P/(N+P)" with boxes \
     "tmp/NAME.all.txt"   using 1:4 title "N/(N+P)" with boxes \
     "tmp/NAME.ham.txt"   using 1:2 title "Ham" with linespoints \
     "tmp/NAME.spam.txt"  using 1:2 title "Spam" with linespoints \
     HAMINTERCEPT/log(10)+x*HAMSLOPE/10000/log(10) title "" with lines  \
     SPAMINTERCEPT/log(10)+x*SPAMSLOPE/10000/log(10) title "" with lines \


