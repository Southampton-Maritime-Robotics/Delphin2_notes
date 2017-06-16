while true; do 
(echo "%CPU %MEM ARGS $(date)" && ps -e -o pcpu,pmem,args --sort=pcpu | cut -d" " -f1-5 | tail) >> cpu.log; sleep 5; 
(echo "%MEM %CPU ARGS $(date)" && ps -e -o pmem,pcpu,args --sort=pmem | cut -d" " -f1-5 | tail) >> memory.log; sleep 5; 
done


