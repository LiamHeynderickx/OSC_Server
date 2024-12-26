make ConnLogBuff
port=5678
clients=3
echo -e "starting gateway "
valgrind --leak-check=full ./ConnBuffLog.out $port $clients & #valgrind --leak-check=full
sleep 3
echo -e 'starting 3 sensor nodes - from shell'
./sensor_node 1 1 127.0.0.1 $port &
sleep 1
./sensor_node 2 3 127.0.0.1 $port &
sleep 2
#wrong port:
./sensor_node 100 2 127.0.0.1 1234 &
sleep 2
#
./sensor_node 3 2 127.0.0.1 $port &
sleep 2
#excess nodes
./sensor_node 4 2 127.0.0.1 $port &
sleep 2
./sensor_node 5 2 127.0.0.1 $port &
sleep 2
./sensor_node 6 2 127.0.0.1 $port &
#terminate
killall sensor_node
sleep 5
killall ConnLogBuff.out

#after running many times valgrind doesnt indicate mem leaks