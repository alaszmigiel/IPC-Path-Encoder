CC = gcc
CFLAGS = -Wall -pthread -Iinclude
SRC_DIR = src
BIN_DIR = bin

SRCS = $(SRC_DIR)/manager.c $(SRC_DIR)/worker1.c $(SRC_DIR)/worker2.c $(SRC_DIR)/worker3.c
OBJS = $(SRCS:.c=.o)

TARGET = manager
WORKERS = worker1 worker2 worker3

all: $(TARGET) $(WORKERS)

$(TARGET): $(SRC_DIR)/manager.o
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC_DIR)/manager.o

worker1: $(SRC_DIR)/worker1.o
	$(CC) $(CFLAGS) -o worker1 $(SRC_DIR)/worker1.o

worker2: $(SRC_DIR)/worker2.o
	$(CC) $(CFLAGS) -o worker2 $(SRC_DIR)/worker2.o

worker3: $(SRC_DIR)/worker3.o
	$(CC) $(CFLAGS) -o worker3 $(SRC_DIR)/worker3.o

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET) $(WORKERS)
