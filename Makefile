CC=clang
CCFLAGS_SHARED = -Wall -Wpedantic -Wextra -Wuninitialized -O2 -pthread -fno-strict-aliasing -std=c23 -Wno-unused-parameter\
		 -I./inc/
CCFLAGS= -g -pg -pthread -fno-strict-aliasing -std=c23 -DDBG -fsanitize=address,undefined
LDFLAGS= -lc
CCFLAGS_PROD=-DPROD -Wno-format -DNOERRCHECK -march=native -ffast-math
CCFLAGS_BENCH=-DPROD -Wno-format -DNOERRCHECK -lprofiler -g
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

all: $(FILES) $(EXEC_FILE)

clean: 
	@rm build/*.o

build/%.o: src/%.c 
	@echo [CC] $@
	@$(CC) $< $(CCFLAGS_SHARED) $(CCFLAGS) -c -o $@

$(EXEC_FILE): $(FILES)
	@echo [CC] $@
	@$(CC) $(wildcard build/*.o) $(CCFLAGS_SHARED) $(CCFLAGS) -o $(EXEC_FILE) $(LDFLAGS)
