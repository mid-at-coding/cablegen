CC=clang
CCFLAGS= -Wall -g -O2 -pg -pthread -lc -Wpedantic -Wextra -Wno-unused-parameter \
		 -fno-strict-aliasing -std=c23 -Wno-unused-command-line-argument \
		 -Wuninitialized -Wno-unused-function -DDBG \
		 -fsanitize=address,undefined
LDFLAGS= -lc -lrt -lm -flto
CCFLAGS_PROD=-Wall -O2 -pthread -DPROD -fno-strict-aliasing -Wno-format \
			  -std=c23 -DNOERRCHECK -march=native
CCFLAGS_BENCH=-Wall -O2 -pthread -DPROD -fno-strict-aliasing -Wno-format \
			  -std=c23 -DNOERRCHECK -lprofiler -g
EXEC_FILE=cablegen
BUILD=debug
FILES=$(addsuffix .o,$(addprefix build/,$(notdir $(basename $(wildcard src/*.c)))))
.PHONY: all clean

ifeq ($(BUILD),prod)
CCFLAGS = $(CCFLAGS_PROD)
endif
ifeq ($(BUILD),bench)
CCFLAGS = $(CCFLAGS_BENCH)
endif

all: $(FILES) cablegen

clean: 
	@rm build/*.o

build/%.o: src/%.c 
	$(CC) $< $(CCFLAGS) -c -o $@ $(LDFLAGS)

cablegen: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS) -o cablegen $(LDFLAGS)
