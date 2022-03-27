CC = gcc
CFLAGS += -std=c99 -Wall -g
INCLUDES = -I ./includes/
LIBS = -lpthread

BUILD_DIR := ./build
OBJ_DIR := ./obj
LIB_DIR := ./libs
SRC_DIR := ./src
TEST_DIR := ./tests
HEADERS_DIR = ./includes/
STUBS_DIR = ./stubs

TARGETS = server client

.DEFAULT_GOAL := all

OBJS_SERVER = obj/worker.o obj/linked_list.o obj/hash_table.o obj/rw_lock.o obj/parser.o obj/cache.o obj/bounded_buffer.o obj/server.o
OBJS_CLIENT = obj/linked_list.o obj/api.o obj/client.o

obj/worker.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c src/worker.c $(LIBS)
	@mv worker.o $(OBJ_DIR)/worker.o

obj/linked_list.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c utils/linked_list.c $(LIBS)
	@mv linked_list.o $(OBJ_DIR)/linked_list.o

obj/hash_table.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c utils/hash_table.c $(LIBS)
	@mv hash_table.o $(OBJ_DIR)/hash_table.o

obj/rw_lock.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c utils/rw_lock.c $(LIBS)
	@mv rw_lock.o $(OBJ_DIR)/rw_lock.o

obj/parser.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c src/parser.c $(LIBS)
	@mv parser.o $(OBJ_DIR)/parser.o

obj/cache.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c src/cache.c $(LIBS)
	@mv cache.o $(OBJ_DIR)/cache.o

obj/bounded_buffer.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c utils/bounded_buffer.c $(LIBS)
	@mv bounded_buffer.o $(OBJ_DIR)/bounded_buffer.o

obj/server.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c src/server.c $(LIBS)
	@mv server.o $(OBJ_DIR)/server.o

obj/api.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c src/api.c $(LIBS)
	@mv api.o $(OBJ_DIR)/api.o

obj/client.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c src/client.c $(LIBS)
	@mv client.o $(OBJ_DIR)/client.o



client: $(OBJS_CLIENT)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/client $(OBJS_CLIENT) $(LIBS)

server: $(OBJS_SERVER)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/server $(OBJS_SERVER) $(LIBS)


test1: client server
	@echo "NUMBER OF WORKER THREADS = 1\nMAX NUMBER OF FILES ACCEPTED = 10000\nMAX CACHE SIZE = 128000000\nSOCKET FILE PATH = $(PWD)/LSOFileStorage.sk\nLOG FILE PATH = $(PWD)/logs/FIFO1.log\nREPLACEMENT POLICY = 0" > config1.txt
	@chmod +x tests/test1.sh
	tests/test1.sh

test2: client server
	@echo "NUMBER OF WORKER THREADS = 4\nMAX NUMBER OF FILES ACCEPTED = 10\nMAX CACHE SIZE = 1000000\nSOCKET FILE PATH = $(PWD)/LSOFileStorage.sk\nLOG FILE PATH = $(PWD)/logs/FIFO2.log\nREPLACEMENT POLICY = 0" > config2.txt
	@chmod +x tests/test2.sh
	tests/test2.sh

test3: client server
	@echo "NUMBER OF WORKER THREADS = 8\nMAX NUMBER OF FILES ACCEPTED = 100\nMAX CACHE SIZE = 32000000\nSOCKET FILE PATH = $(PWD)/LSOFileStorage.sk\nLOG FILE PATH = $(PWD)/logs/FIFO3.log\nREPLACEMENT POLICY = 0" > config3.txt
	@chmod +x tests/test3.sh
	@chmod +x tests/test3_stress.sh
	tests/test3.sh

stats1:
	@chmod +x ./stats.sh
	@echo "\n--------------------FIFO STATS--------------------"
	./stats.sh logs/FIFO1.log
	@echo "\n--------------------LRU STATS--------------------"
	./stats.sh logs/LRU1.log
	@echo "\n--------------------LFU STATS--------------------"
	./stats.sh logs/LFU1.log

stats2:
	@chmod +x ./stats.sh
	@echo "\n--------------------FIFO STATS--------------------"
	./stats.sh logs/FIFO2.log
	@echo "\n--------------------LRU STATS--------------------"
	./stats.sh logs/LRU2.log
	@echo "\n--------------------LFU STATS--------------------"
	./stats.sh logs/LFU2.log

stats3:
	@chmod +x ./stats.sh
	@echo "\n--------------------FIFO STATS--------------------"
	./stats.sh logs/FIFO3.log
	@echo "\n--------------------LRU STATS--------------------"
	./stats.sh logs/LRU3.log
	@echo "\n--------------------LFU STATS--------------------"
	./stats.sh logs/LFU3.log

.PHONY: clean cleanall all stubs
all: $(TARGETS)
clean cleanall:
	rm -rf $(BUILD_DIR)/* $(OBJ_DIR)/* $(LIB_DIR)/* logs/*.log *.sk test1 test2 test3 stubs* *.txt
	@touch $(BUILD_DIR)/.keep
	@touch $(OBJ_DIR)/.keep