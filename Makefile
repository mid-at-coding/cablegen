CC=gcc
CCFLAGS=-Wall -Og -g -pthread -lraylib -lm -flto
CCFLAGS_PROD=-Wall -O3 -pthread -lraylib -lm -DPROD
EXEC_FILE=cablegen
FILES=$(addsuffix .o,$(addprefix build/,$(notdir $(basename $(wildcard src/*.c)))))
.PHONY: all clean

all: $(FILES) cablegen

clean: 
	@rm $(FILES)

build/%.o: src/%.c 
	$(CC) $< $(CCFLAGS) -c -o $@ 

cablegen: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS) -o cablegen

cablegen_prod: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS_PROD) -o cablegen_prod
