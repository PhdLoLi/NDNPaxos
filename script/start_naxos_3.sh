programs=("basic" "quorum" "multi")
client_nums=(1 10 30)
write_ratios=(100 90 10 0)

for program in ${programs[@]}
do
    echo program: ${program}
    for client_num in `seq 1 30`
    #for client_num in ${client_nums[@]}
    do
        for write_ratio in ${write_ratios[@]}
        do
            echo Client_Num: $client_num Write_Ratio: $write_ratio Write and Read From node0
    
            for k in $( seq 0 2 )
            do
            echo node${k}
               nohup ssh -t root@node${k} "cd /home/lijing/NDNPaxos && ./$program ${k} 3 50 1" &
            done
    #        echo  $client_num 
    #        ssh -t root@node4 "cd /home/lijing/naxos &&  bin/clients $client_num $write_ratio 2"
            p_clients="${program}""_clients"
            cd /home/lijing/NDNPaxos &&  ./$p_clients $client_num $write_ratio   #run @node6   

            for k in $( seq 0 2 )
            do
               ssh -t root@node${k} "killall -v $program" 
            done
        done
    done
    cur_time=`date +"%m%d%H"`
    folder_name="${cur_time}""_3r_node0"
    cd /home/lijing/NDNPaxos/results/$program && mkdir $folder_name  && mv *.txt $folder_name
done
