make all
port=5678
clients=3
echo -e "starting gateway "
valgrind --leak-check=full ./sensor_gateway $port $clients &
sleep 3
echo -e 'starting 3 sensor nodes'
./sensor_node 15 10 127.0.0.1 $port & #this node should timeout
sleep 2
./sensor_node 21 3 127.0.0.1 $port &
sleep 2
./sensor_node 37 2 127.0.0.1 $port &
sleep 11
./sensor_node 37 2 127.0.0.1 $port & #excess client is rejected
sleep 11
killall sensor_node
sleep 10
killall sensor_gateway
