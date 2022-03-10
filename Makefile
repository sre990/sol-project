CC = gcc
CFLAGS += -std=c99 -Wall -g
INCLUDES = -I ./includes/
OPT = -lpthread

BUILD_DIR := ./build
OBJ_DIR := ./obj
SRC_DIR := ./src
TEST_DIR := ./tests
HEADERS_DIR = ./includes/
STUBS_DIR = ./stubs

TARGETS = server client

.DEFAULT_GOAL := all

SERVER_OBJS = obj/node.o obj/linked_list.o obj/hash_table.o obj/rw_lock.o obj/parser.o obj/cache.o obj/bounded_buffer.o obj/server.o
CLIENT_OBJS = obj/node.o obj/linked_list.o obj/api.o obj/client.o

obj/node.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c utils/node.c $(OPT)
	@mv node.o $(OBJ_DIR)/node.o

obj/linked_list.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c utils/linked_list.c $(OPT)
	@mv linked_list.o $(OBJ_DIR)/linked_list.o

obj/hash_table.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c utils/hash_table.c $(OPT)
	@mv hash_table.o $(OBJ_DIR)/hash_table.o

obj/rw_lock.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c utils/rw_lock.c $(OPT)
	@mv rw_lock.o $(OBJ_DIR)/rw_lock.o

obj/parser.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c src/parser.c $(OPT)
	@mv parser.o $(OBJ_DIR)/parser.o

obj/cache.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c src/cache.c $(OPT)
	@mv cache.o $(OBJ_DIR)/cache.o

obj/bounded_buffer.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c utils/bounded_buffer.c $(OPT)
	@mv bounded_buffer.o $(OBJ_DIR)/bounded_buffer.o

obj/server.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c src/server.c $(OPT)
	@mv server.o $(OBJ_DIR)/server.o

obj/api.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c src/api.c $(OPT)
	@mv api.o $(OBJ_DIR)/api.o

obj/client.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c src/client.c $(OPT)
	@mv client.o $(OBJ_DIR)/client.o

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/client $(CLIENT_OBJS) $(OPT)

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/server $(SERVER_OBJS) $(OPT)

test1: client server
	@echo "NUMBER OF WORKER THREADS = 1\nMAX NUMBER OF FILES ACCEPTED = 10000\nMAX CACHE SIZE = 128000000\nSOCKET FILE PATH = $(PWD)/LSOfilestorage.sk\nLOG FILE PATH = $(PWD)/logs/FIFO1.log\nREPLACEMENT POLICY = 0" > config1.txt
	@chmod +x tests/test1.sh
	tests/test1.sh

test2: client server
	@echo "NUMBER OF WORKER THREADS = 4\nMAX NUMBER OF FILES ACCEPTED = 10\nMAX CACHE SIZE = 1000000\nSOCKET FILE PATH = $(PWD)/LSOfilestorage.sk\nLOG FILE PATH = $(PWD)/logs/FIFO2.log\nREPLACEMENT POLICY = 0" > config2.txt
	@chmod +x tests/test2.sh
	tests/test2.sh

test3: client server
	@echo "NUMBER OF WORKER THREADS = 8\nMAX NUMBER OF FILES ACCEPTED = 100\nMAX CACHE SIZE = 32000000\nSOCKET FILE PATH = $(PWD)/LSOfilestorage.sk\nLOG FILE PATH = $(PWD)/logs/FIFO3.log\nREPLACEMENT POLICY = 0" > config3.txt
	@chmod +x tests/test3_stress.sh
	@chmod +x tests/test3.sh
	tests/test3_stress.sh

.PHONY: clean cleanall all stubs
all: $(TARGETS)
clean cleanall:
	rm -rf $(BUILD_DIR)/* $(OBJ_DIR)/* logs/*.log *.sk test1 test2 test3 stubs* *.txt
	@touch $(BUILD_DIR)/.keep
	@touch $(OBJ_DIR)/.keep