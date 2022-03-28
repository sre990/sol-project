#!/bin/bash

GREEN="\e[92m"
BLUE="\e[94m"
YELLOW="\e[93m"
BOLD="\e[1m"
RESET="\e[0m"


echo -e "${BOLD}\n--------------------STARTING TEST 3--------------------\n${RESET}"

echo -e "Creating stub files, please wait..."
mkdir stubs1
touch stubs1/stub1.txt
touch stubs1/stub2.txt
touch stubs1/stub3.txt
touch stubs1/stub4.txt
touch stubs1/stub5.txt
touch stubs1/stub6.txt
touch stubs1/stub7.txt
touch stubs1/stub8.txt
touch stubs1/stub9.txt
touch stubs1.stub10.txt

echo -ne '[------------------]       (0%)\r'
sleep 1
head -c 1MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub1.txt
head -c 5MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub2.txt
head -c 10MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub3.txt
echo -ne '[#####-------------]      (33%)\r'
sleep 1
head -c 15MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub4.txt
head -c 20MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub5.txt
head -c 25MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub6.txt
echo -ne '[#############-------]      (66%)\r'
sleep 1
head -c 28MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub7.txt
head -c 29MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub8.txt
echo -ne '[################----]      (80%)\r'
sleep 1
head -c 30MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub9.txt
head -c 31MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub10.txt
echo -ne '[####################]     (100%)\r'
echo -ne '\n'
cp -r stubs1 stubs2
cp -r stubs1 stubs3
cp -r stubs1 stubs4
cp -r stubs1 stubs5
cp -r stubs1 stubs6
cp -r stubs1 stubs7
cp -r stubs1 stubs8
cp -r stubs1 stubs9
cp -r stubs1 stubs10


echo -e "${BLUE}FIFO> Starting up the server...${RESET}"
build/server ./config3.txt &
# server pid
SERVER=$!
export SERVER
sleep 3s

bash -c 'sleep 30 && kill -2 ${SERVER}' &
echo -e "${BLUE}FIFO> Shutting down the server with SIGINT in 30s...${RESET}"

# start 10 times stress test
pids=()
for i in {1..10}; do
	bash -c 'tests/test3_stress.sh' &
	pids+=($!)
	sleep 0.1
done

# sleep 30 seconds and kill all instances
sleep 30s

for i in "${pids[@]}"; do
	kill ${i}
	wait ${i} 2>/dev/null
done

wait $SERVER
# kill all clients quietly
killall -q build/client
echo -e "${BLUE}FIFO> Finished.
${RESET}"

# changing config file name, changing to LRU replacement policy
sed -i '$s/0/1/' config3.txt
# change log file name
sed -i -e 's/FIFO3.log/LRU3.log/g' config3.txt

echo -e "${YELLOW}LRU> Starting up the server...${RESET}"
build/server ./config3.txt &
# server pid
SERVER=$!
export SERVER
sleep 3s

bash -c 'sleep 30 && kill -2 ${SERVER}' &
echo -e "${YELLOW}LRU> Shutting down the server with SIGINT in 30s...${RESET}"

# start 10 times stress test
pids=()
for i in {1..10}; do
	bash -c 'tests/test3_stress.sh' &
	pids+=($!)
	sleep 0.1
done

# sleep 30 seconds and kill all instances
sleep 30s

for i in "${pids[@]}"; do
	kill ${i}
	wait ${i} 2>/dev/null
done

wait $SERVER
# kill all clients quietly
killall -q build/client
echo -e "${YELLOW}LRU> Finished.
${RESET}"

# changing config file name, changing to LFU replacement policy
sed -i '$s/1/2/' config3.txt
# change log file name
sed -i -e 's/LRU3.log/LFU3.log/g' config3.txt

echo -e "${GREEN}LFU> Starting up the server...${RESET}"

build/server ./config3.txt &
# server pid
SERVER=$!
export SERVER
sleep 3s

bash -c 'sleep 30 && kill -2 ${SERVER}' &
echo -e "${GREEN}LFU> Shutting down the server with SIGINT in 30s...${RESET}"

# start 10 times stress test
pids=()
for i in {1..10}; do
	bash -c 'tests/test3_stress.sh' &
	pids+=($!)
	sleep 0.1
done

# sleep 30 seconds and kill all instances
sleep 30s

for i in "${pids[@]}"; do
	kill ${i}
	wait ${i} 2>/dev/null
done

wait $SERVER
# kill all clients quietly
killall -q build/client
echo -e "${GREEN}LFU> Finished.
${RESET}"

rm -rf stubs* config3.txt

echo -e "${BOLD}--------------------TEST 3 HAS FINISHED--------------------\n${RESET}"

exit 0
