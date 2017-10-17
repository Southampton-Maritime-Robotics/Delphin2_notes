while true; do 
(echo "%CPU %MEM ARGS $(date)" && ps -e -o pcpu,pmem,args --sort=pcpu | cut -d" " -f1-5 | tail) >> cpu.log; 
(echo "Total CPU usage *10: " && ps aux | sed 's/\.//' | tail -n +2 | awk '{sum+=$3} END {print sum}') >> cpu.log;
(echo "%MEM %CPU ARGS $(date)" && ps -e -o pmem,pcpu,args --sort=pmem | cut -d" " -f1-5 | tail) >> memory.log; 
(echo && free -m) >> memory.log
sleep 2; 
done


