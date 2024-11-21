CC = g++

HEADER_DIR = include

CFLAGS = -Wall -Werror -std=c11 -w -I ${HEADER_DIR}

OUTPUT_DIR = bin
EXE_BUILD = node

MAIN = main
SRC_MAIN = main.cpp
OBJ_MAIN = $(SRC_MAIN:.cpp=.o)

SRC_DIR = src

SRC = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)$(wildcard $(SRC_DIR)/*/*/*.cpp)
OBJS = $(patsubst %,$(OUTPUT_DIR)/%,$(SRC:.cpp=.o))

.PHONY: all clean run

all: build run

build: $(OUTPUT_DIR)/$(OBJ_MAIN) $(OBJS)
	$(info [Build Program])
	@echo -n ">> "
	$(CC) $(CFLAGS) -o $(EXE_BUILD) $^

$(OUTPUT_DIR)/%.o: %.cpp
	@echo -n ">> "
	mkdir -p $(@D)
	@echo -n ">> "
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(info [Clean Program])
	@echo -n ">> "
	rm -f $(OUTPUT_DIR)/$(OBJ_MAIN) $(OUTPUT_DIR)/$(MAIN) $(OBJS) $(EXE_BUILD)



RUN_ARGS = localhost 3000

run:
	$(info [Run Program])
	@echo -n ">> "
	./$(EXE_BUILD) $(RUN_ARGS)
	
