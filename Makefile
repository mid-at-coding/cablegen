CC=gcc
CCFLAGS=-Wall -Og -g -pg -pthread -lm -Wpedantic -Wextra -Wno-unused -Wno-format -fno-strict-aliasing
CCFLAGS_PROD=-Wall -O2 -static -pthread -lm -DPROD --static -fno-strict-aliasing -Wno-format
EXEC_FILE=cablegen
FILES=$(addsuffix .o,$(addprefix build/,$(notdir $(basename $(wildcard src/*.c)))))
.PHONY: all clean

all: $(FILES) cablegen

clean: 
	@rm build/*.o

build/%.o: src/%.c 
	$(CC) $< $(CCFLAGS_PROD) -c -o $@ 

cablegen: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS) -o cablegen

cablegen_prod: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS_PROD) -o cablegen_prod
