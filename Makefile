CC=gcc
CCFLAGS= -Wall -g -O2 -pg -pthread -lm -lc -Wpedantic -Wextra -Wno-unused-parameter \
		 -Wno-format -fno-strict-aliasing -std=c23 -Wno-unused-command-line-argument \
		 -Wuninitialized -fsanitize=address
CCFLAGS_PROD=-Wall -O2 -static -pthread -lm -lc -DPROD --static -fno-strict-aliasing -Wno-format \
			  -std=c23 -Wno-unused-parameter -Wno-unused-command-line-argument -Wno-unused-function
EXEC_FILE=cablegen
FILES=$(addsuffix .o,$(addprefix build/,$(notdir $(basename $(wildcard src/*.c)))))
.PHONY: all clean

all: $(FILES) cablegen

clean: 
	@rm build/*.o

build/%.o: src/%.c 
	$(CC) $< $(CCFLAGS) -c -o $@ 

cablegen: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS) -o cablegen

cablegen_prod: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS_PROD) -o cablegen
