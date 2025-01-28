CC=gcc
CCFLAGS=-Wall -Og -g -pthread
CCFLAGS_PROD=-Wall -O3 -pthread
EXEC_FILE=tablegen
FILES=$(addsuffix .o,$(addprefix build/,$(notdir $(basename $(wildcard src/*.c)))))
.PHONY: all clean

all: $(FILES) tablegen

clean: 
	@rm $(FILES)

build/%.o: src/%.c 
	$(CC) $< $(CCFLAGS) -c -o $@ 

tablegen: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS) -o tablegen

tablegen_prod: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS) -o tablegen
