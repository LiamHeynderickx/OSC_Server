make conn
port=5678
clients=3
echo -e "starting gateway "
valgrind valgrind --leak-check=full ./connmgr.out $port $clients &
sleep 3
echo -e 'starting 3 sensor nodes'
./sensor_node 15 1 127.0.0.1 $port &
sleep 2
./sensor_node 21 3 127.0.0.1 $port &
sleep 2
./sensor_node 37 2 127.0.0.1 $port &
sleep 2
#excess nodes
./sensor_node 10 2 127.0.0.1 $port &
sleep 2
./sensor_node 11 2 127.0.0.1 $port &
sleep 2
./sensor_node 12 2 127.0.0.1 $port &
#terminate
killall sensor_node
sleep 5
killall connmgr.out

#after running many times valgrind doesnt indicate mem leaks