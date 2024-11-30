CC = g++

HEADER_DIR = include

CFLAGS = -Wall -Werror -std=c++11 -w -I ${HEADER_DIR}

OUTPUT_DIR = bin
EXE_BUILD = node

MAIN = main
SRC_MAIN = main.cpp
OBJ_MAIN = $(SRC_MAIN:.cpp=.o)

SRC_DIR = src

SRC = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)$(wildcard $(SRC_DIR)/*/*/*.cpp)
OBJS = $(patsubst %,$(OUTPUT_DIR)/%,$(SRC:.cpp=.o))

.PHONY: all clean run

all: build run debugbuild


$(OUTPUT_DIR)/%.o: %.cpp
	@echo -n ">> "
	mkdir -p $(@D)
	@echo -n ">> "
	$(CC) $(CFLAGS) -c -o $@ $<

build: $(OUTPUT_DIR)/$(OBJ_MAIN) $(OBJS)
	$(info [Build Program])
	@echo -n ">> "
	$(CC) $(CFLAGS) -o $(EXE_BUILD) $^

RUN_ARGS = localhost 3000

run:
	$(info [Run Program])
	@echo -n ">> "
	./$(EXE_BUILD) $(RUN_ARGS)


DEBUG_FLAGS = -g
DEBUG_OUTPUT_DIR = debug-bin
DEBUG_OBJS = $(patsubst %,$(DEBUG_OUTPUT_DIR)/%,$(SRC:.cpp=.o))
$(DEBUG_OUTPUT_DIR)/%.o: %.cpp
	@echo -n ">> "
	mkdir -p $(@D)
	@echo -n ">> "
	$(CC) $(DEBUG_FLAGS) $(CFLAGS) -c -o $@ $<

debugbuild: $(DEBUG_OUTPUT_DIR)/$(OBJ_MAIN) $(DEBUG_OBJS)
	$(info [Build Program])
	@echo -n ">> "
	$(CC) $(DEBUG_FLAGS) $(CFLAGS) -o $(EXE_BUILD) $^





clean:
	$(info [Clean Program])
	@echo -n ">> "
	rm -f $(OUTPUT_DIR)/$(OBJ_MAIN) $(OUTPUT_DIR)/$(MAIN) $(OBJS) $(EXE_BUILD) $(DEBUG_OUTPUT_DIR)/$(OBJ_MAIN) $(DEBUG_OUTPUT_DIR)/$(MAIN) $(DEBUG_OBJS)



	
