CC=clang
CCFLAGS_SHARED = -Wall -Wpedantic -Wextra -Wuninitialized -O2 -pthread -fno-strict-aliasing -std=c23 -Wno-unused-parameter\
		 -I./inc/ -I./inc/external
CCFLAGS= -g -pg -pthread -fno-strict-aliasing -std=c23 -DDBG -fsanitize=address,undefined
CCFLAGS_PROD=-DPROD -Wno-format -DNOERRCHECK -march=native -ffast-math
CCFLAGS_BENCH=-DPROD -Wno-format -DNOERRCHECK -DBENCH
EXEC_FILE=cablegen
BUILD=debug
PLATFORM=linux
LDFLAGS=-lraylib
FILES=$(addsuffix .o,$(addprefix build/,$(notdir $(basename $(wildcard src/*.c)))))
CCFLAGS_SHARED += -DVERSION=$$(git describe --tags --always --dirty)
.PHONY: all clean

ifeq ($(PLATFORM),windows)
CC=x86_64-w64-mingw32-gcc
LDFLAGS += -lm
endif
ifeq ($(PLATFORM),linux)
LDFLAGS += -lm -lc
endif

ifeq ($(BUILD),prod)
CCFLAGS = $(CCFLAGS_PROD)
endif
ifeq ($(BUILD),bench)
CCFLAGS = $(CCFLAGS_BENCH)
endif

all: build/ini.o $(FILES) $(EXEC_FILE)

clean: 
	@rm build/*.o

build/ini.o: src/external/ini.c 
	@echo [CC] $@
	@$(CC) $< $(CCFLAGS_SHARED) $(CCFLAGS) -c -o $@

build/ui.o: src/ui.c 
	@echo [CC] $@
	@$(CC) $< $(CCFLAGS_SHARED) $(CCFLAGS) -Wno-missing-field-initializers -Wno-missing-braces -c -o $@

build/%.o: src/%.c 
	@echo [CC] $@
	@$(CC) $< $(CCFLAGS_SHARED) $(CCFLAGS) -c -o $@

$(EXEC_FILE): $(FILES)
	@echo [CC] $@
	@$(CC) $(wildcard build/*.o) $(CCFLAGS_SHARED) $(CCFLAGS) -o $(EXEC_FILE) $(LDFLAGS)
