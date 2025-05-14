CC=x86_64-w64-mingw32-gcc
CCFLAGS=-Wall -Og -g -pthread -lm -Wpedantic -Wextra -Wno-unused
CCFLAGS_PROD=-Wall -O3 -pthread -lm -DPROD
EXEC_FILE=cablegen
FILES=$(addsuffix .o,$(addprefix build/,$(notdir $(basename $(wildcard src/*.c)))))
.PHONY: all clean

all: $(FILES) cablegen

clean: 
	@rm $(FILES)

build/%.o: src/%.c 
	$(CC) $< $(CCFLAGS_PROD) -c -o $@ 

cablegen: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS) -o cablegen

cablegen_prod: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS_PROD) -o cablegen_prod
