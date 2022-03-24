#!/bin/bash

GREEN="\e[92m"
BLUE="\e[94m"
YELLOW="\e[93m"
BOLD="\e[1m"
RESET="\e[0m"

# starting
echo -e "${BOLD}\n--------------------STARTING TEST 1--------------------\n${RESET}"
# creating stubs
echo -e "Creating stub files, please wait..."
mkdir stubs
touch stubs/stub1.txt
touch stubs/stub2.txt
touch stubs/stub3.txt
touch stubs/stub4.txt
touch stubs/stub5.txt
touch stubs/stub6.txt
touch stubs/stub7.txt
touch stubs/stub7.txt
touch stubs/stub8.txt
touch stubs/stub9.txt
touch stubs/stub10.txt

echo -ne '[------------------]       (0%)\r'
sleep 1
head -c 10MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs/stub1.txt
head -c 20MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs/stub2.txt
head -c 30MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs/stub3.txt
echo -ne '[#####-------------]      (33%)\r'
sleep 1
head -c 40MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs/stub4.txt
head -c 50MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs/stub5.txt
head -c 55MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs/stub6.txt
echo -ne '[#############-------]      (66%)\r'
sleep 1
head -c 60MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs/stub7.txt
head -c 70MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs/stub8.txt
echo -ne '[################----]      (80%)\r'
sleep 1
head -c 80MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs/stub9.txt
head -c 90MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs/stub10.txt
echo -ne '[####################]     (100%)\r'
echo -ne '\n'

# booting the server
echo -e "${BLUE}FIFO> Starting up the server...${RESET}"
valgrind --leak-check=full build/server ./config1.txt &
# server pid
SERVER_PID=$!
export SERVER_PID

sleep 5s

server_file="$(pwd)/src/server.c"
parser_file="$(pwd)/src/parser.c"

echo -e "${BLUE}FIFO> Setting up clients${RESET}"
# print help message
build/client -h
# connect
build/client -p -t 200 -f LSOfiletorage.sk
# send directory, store victims if any
build/client -p -t 200 -f LSOfiletorage.sk -w stubs -D test1/FIFO/evicted
# send directory, store victims if any
build/client -p -t 200 -f LSOfiletorage.sk -w src -D test1/FIFO/evicted
# read every file inside server, send to directory, unlock a file
build/client -p -t 200 -f LSOfiletorage.sk -R 0 -d test1/FIFO/read -u ${parser_file}
# write a file inside cache, lock it, delete a file
build/client -p -t 200 -f LSOfiletorage.sk -W ${server_file} -l ${server_file} -r ${parser_file},${server_file} -c ${server_file}

echo -e "${BLUE}FIFO> Shutting down the server with SIGHUP...${RESET}"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo -e "${BLUE}FIFO> Finished.
${RESET}"

# replace 0 with 1 in config1.txt => use LRU replacement policy
sed -i '$s/0/1/' config1.txt
# change log file name
sed -i -e 's/FIFO1.log/LRU1.log/g' config1.txt

echo -e "${YELLOW}LRU> Starting up the server...${RESET}"

valgrind --leak-check=full build/server ./config1.txt &
# server pid
SERVER_PID=$!
export SERVER_PID

sleep 5s

echo -e "${YELLOW}LRU> Setting up clients${RESET}"
# print help message
build/client -h
# connect
build/client -p -t 200 -f LSOfiletorage.sk
# send directory, store victims if any
build/client -p -t 200 -f LSOfiletorage.sk -w stubs -D test1/LRU/evicted
# send directory, store victims if any
build/client -p -t 200 -f LSOfiletorage.sk -w src -D test1/LRU/evicted
# read every file inside server, send to directory, unlock a file
build/client -p -t 200 -f LSOfiletorage.sk -R 0 -d test1/LRU/read -u ${parser_file}
# write a file inside cache, lock it, delete a file
build/client -p -t 200 -f LSOfiletorage.sk -W ${server_file} -l ${server_file} -r ${parser_file},${server_file} -c ${server_file}

echo -e "${YELLOW}LRU> Shutting down the server with SIGHUP...${RESET}"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo -e "${YELLOW}LRU> Finished.
${RESET}"

# replace 1 with 2 in config1.txt => use LFU replacement policy
sed -i '$s/1/2/' config1.txt
# change log file name
sed -i -e 's/LRU1.log/LFU1.log/g' config1.txt

echo -e "${GREEN}LFU> Starting up the server...${RESET}"

valgrind --leak-check=full build/server ./config1.txt &
# server pid
SERVER_PID=$!
export SERVER_PID

sleep 5s

echo -e "${GREEN}LFU> Setting up clients${RESET}"
# print help message
build/client -h
# connect
build/client -p -t 200 -f LSOfiletorage.sk
# send directory, store victims if any
build/client -p -t 200 -f LSOfiletorage.sk -w stubs -D test1/LFU/evicted
# send directory, store victims if any
build/client -p -t 200 -f LSOfiletorage.sk -w src -D test1/LFU/evicted
# read every file inside server, send to directory, unlock a file
build/client -p -t 200 -f LSOfiletorage.sk -R 0 -d test1/LFU/read -u ${parser_file}
# write a file inside cache, lock it, delete a file
build/client -p -t 200 -f LSOfiletorage.sk -W ${server_file} -l ${server_file} -r ${parser_file},${server_file} -c ${server_file}

echo -e "${GREEN}LFU> Shutting down the server with SIGHUP...${RESET}"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo -e "${GREEN}LFU> Finished.
${RESET}"
echo -e "${BOLD}--------------------TEST 1 HAS FINISHED--------------------\n${RESET}"
rm config1.txt
rm -r stubs
exit 0