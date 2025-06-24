CC=clang
CCFLAGS= -Wall -g -O2 -pg -pthread -lc -Wpedantic -Wextra -Wno-unused-parameter \
		 -fno-strict-aliasing -std=c99 -Wno-unused-command-line-argument \
		 -Wuninitialized -fcolor-diagnostics -Wno-unused-function
CCFLAGS_PROD=-Wall -O2 -pthread -DPROD -fno-strict-aliasing -Wno-format \
			  -std=c99
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
	$(CC) $(wildcard build/*.o) $(CCFLAGS_PROD) -o cablegen
