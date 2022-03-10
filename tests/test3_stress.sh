#!/bin/bash

BLUE="\e[1;34m"
YELLOW="\033[1;33m"
GREEN="\033[1;32m"
RESET="\033[0m"

# starting
echo -e "\t\tSTARTING TARGET TEST3"

echo -e "Creating stubs, please wait..."
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

echo -ne '[---------------------]    (0%)\r'
sleep 1
head -c 1MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub1.txt
head -c 5MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub2.txt
head -c 9MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub3.txt
echo -ne '[#######--------------]    (33%)\r'
sleep 1
head -c 15MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub4.txt
head -c 18MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub5.txt
head -c 22MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub6.txt
echo -ne '[#############--------]    (66%)\r'
sleep 1
head -c 28MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub7.txt
head -c 29MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub8.txt
head -c 30MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub9.txt
head -c 31MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub10.txt
echo -ne '[####################]    (100%)\r'
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

# Starting server
echo -e "${BLUE}[FIFO] Starting server${RESET}"
build/server ./config3.txt &
# server pid
SERVER_PID=$!
export SERVER_PID
sleep 5s

bash -c 'sleep 30 && kill -2 ${SERVER_PID}' &
echo -e "${BLUE}[FIFO] Sending SIGINT, server will shutdown in 30s${RESET}"

# start 10 times test3 script
pids=()
for i in {1..10}; do
	bash -c 'tests/test3.sh' &
	pids+=($!)
	sleep 0.1
done

sleep 30s

# kill spawned instances
for i in "${pids[@]}"; do
	kill ${i}
	wait ${i} 2>/dev/null
done

wait $SERVER_PID
killall -q build/client # kill every running client
echo -e "${BLUE}[FIFO] Test has finished\n${RESET}"

# replace 0 with 1 in config3.txt => use LRU replacement policy
sed -i '$s/0/1/' config3.txt
# change log file name
sed -i -e 's/FIFO3.log/LRU3.log/g' config3.txt

echo -e "${YELLOW}[LRU] Starting server${RESET}"
build/server ./config3.txt &
# server pid
SERVER_PID=$!
export SERVER_PID
sleep 5s

bash -c 'sleep 30 && kill -2 ${SERVER_PID}' &
echo -e "${YELLOW}[LRU] Sending SIGINT, server will shutdown in 30s${RESET}"

# start 10 times aux script
pids=()
for i in {1..10}; do
	bash -c 'tests/test3.sh' &
	pids+=($!)
	sleep 0.1
done

sleep 30s

# kill spawned instances
for i in "${pids[@]}"; do
	kill ${i}
	wait ${i} 2>/dev/null
done

wait $SERVER_PID
killall -q build/client # kill every running client
echo -e "${YELLOW}[LRU] Test has finished\n${RESET}"

# replace 1 with 2 in config3.txt => use LFU replacement policy
sed -i '$s/1/2/' config3.txt
# change log file name
sed -i -e 's/LRU3.log/LFU3.log/g' config3.txt

echo -e "${GREEN}[LFU] Starting server${RESET}"

build/server ./config3.txt &
# server pid
SERVER_PID=$!
export SERVER_PID
sleep 5s

bash -c 'sleep 30 && kill -2 ${SERVER_PID}' &
echo -e "${GREEN}[LFU] Sending SIGINT, server will shutdown in 30s${RESET}"

# start 10 times aux script
pids=()
for i in {1..10}; do
	bash -c 'tests/test3.sh' &
	pids+=($!)
	sleep 0.1
done

sleep 30s

# kill spawned instances
for i in "${pids[@]}"; do
	kill ${i}
	wait ${i} 2>/dev/null
done

wait $SERVER_PID
killall -q build/client # kill every running client
echo -e "${GREEN}[LFU] Test has finished\n${RESET}"

rm -rf stubs* config3.txt

echo -e "\t\tTARGET TEST3 HAS FINISHED\n"

exit 0
