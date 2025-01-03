make allTog #for testing
port=5678
clients=3
echo -e "starting gateway "
valgrind --leak-check=full ./allTog.out $port $clients & #valgrind --leak-check=full
sleep 3
echo -e 'starting 3 sensor nodes - from shell'
./sensor_node 15 1 127.0.0.1 $port &
sleep 1
./sensor_node 21 3 127.0.0.1 $port &
sleep 2
#wrong port:
./sensor_node 37 2 127.0.0.1 1234 &
sleep 2
#
./sensor_node 49 2 127.0.0.1 $port &
sleep 2
#excess nodes
./sensor_node 11 2 127.0.0.1 $port &
sleep 2
./sensor_node 12 2 127.0.0.1 $port &
sleep 2
./sensor_node 13 2 127.0.0.1 $port &
#terminate
killall sensor_node
sleep 10
killall allTog.out
