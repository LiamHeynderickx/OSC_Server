make all
port=5678
clients=5
echo -e "starting gateway "
./sensor_gateway $port $clients &
sleep 3
echo -e 'starting 5 sensor nodes'
./sensor_node 15 1 127.0.0.1 $port &

./sensor_node 21 3 127.0.0.1 $port &

./sensor_node 37 2 127.0.0.1 $port &

./sensor_node 132 3 127.0.0.1 $port &

./sensor_node 142 3 127.0.0.1 $port &
sleep 11
killall sensor_node
sleep 30
killall sensor_gateway
