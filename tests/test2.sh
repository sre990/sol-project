#!/bin/bash

BLUE="\e[1;34m"
YELLOW="\033[1;33m"
GREEN="\033[1;32m"
RESET="\033[0m"

# starting
echo -e "\t\tSTARTING TARGET TEST2"

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
touch stubs1/stub10.txt

echo -ne '[---------------------]    (0%)\r'
sleep 1
head -c 100KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub1.txt
head -c 200KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub2.txt
head -c 300KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub3.txt
echo -ne '[#######--------------]    (33%)\r'
sleep 1
head -c 350KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub4.txt
head -c 400KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub5.txt
head -c 450KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub6.txt
echo -ne '[#############--------]    (66%)\r'
sleep 1
head -c 700KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub7.txt
head -c 800KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub8.txt
head -c 900KB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub9.txt
head -c 1MB /dev/urandom | tr -dc 'a-zA-Z0-9~!@#$%^&*_-' | fold > stubs1/stub10.txt
echo -ne '[####################]    (100%)\r'
echo -ne '\n'

cp -r stubs1 stubs2
cp -r stubs2 stubs3

# booting the server
echo -e "${BLUE}[FIFO] Starting the server${RESET}"
build/server ./config2.txt &
# server pid
SERVER_PID=$!
export SERVER_PID
sleep 5s

echo -e "${BLUE}[FIFO] Starting clients${RESET}"
# connect, send files, save victims if any
build/client -p -t 10 -f LSOfilestorage.sk -w stubs1 -D ./test2/FIFO/victims1
# connect, send files, save victims if any
build/client -p -t 10 -f LSOfilestorage.sk -w stubs2 -D ./test2/FIFO/victims2
# connect, send files, save victims if any
build/client -p -t 10 -f LSOfilestorage.sk -w stubs3 -D ./test2/FIFO/victims3

echo -e "${BLUE}[FIFO] Sending SIGHUP to server${RESET}"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo -e "${BLUE}[FIFO] Test has finished\n${RESET}"

# replace 0 with 1 in config2.txt => use LRU replacement policy
sed -i '$s/0/1/' config2.txt
# change log file name
sed -i -e 's/FIFO2.log/LRU2.log/g' config2.txt

echo -e "${YELLOW}[LRU] Starting the server${RESET}"

build/server ./config2.txt &
# server pid
SERVER_PID=$!
export SERVER_PID

sleep 5s

echo -e "${YELLOW}[LRU] Starting clients${RESET}"
# connect, send files, save victims if any
build/client -p -t 10 -f LSOfilestorage.sk -w stubs1 -D ./test2/LRU/victims1
# connect, send files, save victims if any
build/client -p -t 10 -f LSOfilestorage.sk -w stubs2 -D ./test2/LRU/victims2
# connect, send files, save victims if any
build/client -p -t 10 -f LSOfilestorage.sk -w stubs3 -D ./test2/LRU/victims3

echo -e "${YELLOW}[LRU] Sending SIGHUP to server${RESET}"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo -e "${YELLOW}[LRU] Test has finished\n${RESET}"

# replace 1 with 2 in config2.txt => use LFU replacement policy
sed -i '$s/1/2/' config2.txt
# change log file name
sed -i -e 's/LRU2.log/LFU2.log/g' config2.txt

echo -e "${GREEN}[LFU] Starting the server${RESET}"

build/server ./config2.txt &
# server pid
SERVER_PID=$!
export SERVER_PID

sleep 5s

echo -e "${GREEN}[LFU] Starting clients${RESET}"
# connect, send files, save victims if any
build/client -p -t 10 -f LSOfilestorage.sk -w stubs1 -D ./test2/LFU/victims1
# connect, send files, save victims if any
build/client -p -t 10 -f LSOfilestorage.sk -w stubs2 -D ./test2/LFU/victims2
# connect, send files, save victims if any
build/client -p -t 10 -f LSOfilestorage.sk -w stubs3 -D ./test2/LFU/victims3

echo -e "${GREEN}[LFU] Sending SIGHUP to server${RESET}"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo -e "${GREEN}[LFU] Test has finished\n${RESET}"
echo -e "\t\tTARGET TEST2 HAS FINISHED\n"

rm config2.txt
rm -r stubs1 stubs2 stubs3
exit 0
