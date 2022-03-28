#!/bin/bash

GREEN="\e[92m"
BLUE="\e[94m"
YELLOW="\e[93m"
BOLD="\e[1m"
RESET="\e[0m"


echo -e "${BOLD}\n--------------------STARTING TEST 2--------------------\n${RESET}"

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
touch stubs1/stub10.txt

echo -ne '[------------------]       (0%)\r'
sleep 1
head -c 250KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub1.txt
head -c 300KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub2.txt
head -c 350KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub3.txt
echo -ne '[#####-------------]      (33%)\r'
sleep 1
head -c 400KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub4.txt
head -c 500KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub5.txt
head -c 600KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub6.txt
echo -ne '[#############-------]      (66%)\r'
sleep 1
head -c 700KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub7.txt
head -c 800KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub8.txt
echo -ne '[################----]      (80%)\r'
sleep 1
head -c 900KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub9.txt
head -c 1MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub10.txt
echo -ne '[####################]     (100%)\r'
echo -ne '\n'
cp -r stubs1 stubs2
cp -r stubs2 stubs3


echo -e "${BLUE}FIFO> Starting up the server...${RESET}"
build/server ./config2.txt &
# server pid
SERVER=$!
export SERVER
sleep 3s

echo -e "${BLUE}FIFO> Setting up clients...${RESET}"
# connect, send files, save victims if any
build/client -p -t 10 -f LSOFileStorage.sk -w stubs1 -D test2/FIFO/evicted1
# connect, send files, save victims if any
build/client -p -t 10 -f LSOFileStorage.sk -w stubs2 -D test2/FIFO/evicted2
# connect, send files, save victims if any
build/client -p -t 10 -f LSOFileStorage.sk -w stubs3 -D test2/FIFO/evicted3

echo -e "${BLUE}FIFO> Shutting down the server with SIGHUP...${RESET}"
kill -s SIGHUP $SERVER
wait $SERVER
echo -e "${BLUE}FIFO> Finished.
${RESET}"

# changing config file name, changing to LRU replacement policy
sed -i '$s/0/1/' config2.txt
# change log file name
sed -i -e 's/FIFO2.log/LRU2.log/g' config2.txt

echo -e "${YELLOW}LRU> Starting up the server...${RESET}"

build/server ./config2.txt &
# server pid
SERVER=$!
export SERVER

sleep 3s

echo -e "${YELLOW}LRU> Setting up clients...${RESET}"
# connect, send files, save victims if any
build/client -p -t 10 -f LSOFileStorage.sk -w stubs1 -D test2/LRU/evicted1
# connect, send files, save victims if any
build/client -p -t 10 -f LSOFileStorage.sk -w stubs2 -D test2/LRU/evicted2
# connect, send files, save victims if any
build/client -p -t 10 -f LSOFileStorage.sk -w stubs3 -D test2/LRU/evicted3

echo -e "${YELLOW}LRU> Shutting down the server with SIGHUP...${RESET}"
kill -s SIGHUP $SERVER
wait $SERVER
echo -e "${YELLOW}LRU> Finished.
${RESET}"

# changing config file name, changing to LFU replacement policy
sed -i '$s/1/2/' config2.txt
# change log file name
sed -i -e 's/LRU2.log/LFU2.log/g' config2.txt

echo -e "${GREEN}LFU> Starting up the server...${RESET}"

build/server ./config2.txt &
# server pid
SERVER=$!
export SERVER

sleep 3s

echo -e "${GREEN}LFU> Setting up clients...${RESET}"
# connect, send files, save victims if any
build/client -p -t 10 -f LSOFileStorage.sk -w stubs1 -D test2/LFU/evicted1
# connect, send files, save victims if any
build/client -p -t 10 -f LSOFileStorage.sk -w stubs2 -D test2/LFU/evicted2
# connect, send files, save victims if any
build/client -p -t 10 -f LSOFileStorage.sk -w stubs3 -D test2/LFU/evicted3

echo -e "${GREEN}LFU> Shutting down the server with SIGHUP...${RESET}"
kill -s SIGHUP $SERVER
wait $SERVER
echo -e "${GREEN}LFU> Finished.
${RESET}"
echo -e "${BOLD}--------------------TEST 2 HAS FINISHED--------------------\n${RESET}"

rm config2.txt
rm -r stubs1 stubs2 stubs3
exit 0
