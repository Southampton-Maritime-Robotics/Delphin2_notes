# 2017-10-17
# ps commands need swapping in favour of top
# based on top giving a better time resolution
# https://unix.stackexchange.com/questions/58539/top-and-ps-not-showing-the-same-cpu-result
# https://www.howtogeek.com/194642/understanding-the-load-average-on-linux-and-other-unix-like-systems/
# CAUTION: Columns for top vary with the version of top, so they might vary between systems
# however, currently top is so outdated that it cannot give this kind of output

while true; do 
(echo "%CPU %MEM ARGS $(date)" && ps -e -o pcpu,pmem,args --sort=pcpu | cut -d" " -f1-5 | tail) >> cpu.log; 
(echo "Total CPU usage *10: " && ps aux | sed 's/\.//' | tail -n +2 | awk '{sum+=$3} END {print sum}') >> cpu.log;
(echo "%MEM %CPU ARGS $(date)" && ps -e -o pmem,pcpu,args --sort=pmem | cut -d" " -f1-5 | tail) >> memory.log; 
(echo && free -m) >> memory.log
sleep 2; 
done


#top cpu usage:
#top -o "%CPU" -bn1 | tail -n +9 | head | awk '{print $7 " " $8 " " $NF}'
#
#top mem usage
#top -o "%MEM" -bn1 | tail -n +9 | head | awk '{print $8 " " $7 " " $NF}'
#

