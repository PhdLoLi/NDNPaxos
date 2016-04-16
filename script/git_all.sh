for k in $( seq 0 6 )
do
   nohup ssh lijing@node${k} 'bash -s' < git.sh &
done

#   ssh lijing@node5 'bash -s' < git.sh 
cd /home/lijing/NDNPaxos && git stash && git pull && ./waf configure -l info && ./waf && cp bin/naxos ../basic && ./waf clean && ./waf configure -l info -m Q && cp bin/naxos ../quorum && ./waf clean && ./waf configure -l info -m M && ./waf && cp bin/naxos ../multi
