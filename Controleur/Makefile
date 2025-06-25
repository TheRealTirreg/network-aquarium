# Variables
CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -lm -lncurses # math lib and ncurses for terminal UI
SRC_DIR = src
BIN_DIR = bin
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)
EXECUTABLE = $(BIN_DIR)/serveur

# Cibles
all: compile run

compile: $(BIN_DIR) $(EXECUTABLE)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(EXECUTABLE): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJ_FILES) $(LDFLAGS)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/handle_client.h
	$(CC) $(CFLAGS) -c $< -o $@

run:
	$(EXECUTABLE)

valgrind: compile
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --log-file=valgrind.log $(EXECUTABLE)

clean:
	rm -rf $(BIN_DIR)

.PHONY: all compile run clean