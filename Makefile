CC=clang
CCFLAGS_SHARED = -Wall -Wpedantic -Wextra -Wuninitialized -O2 -pthread -fno-strict-aliasing -std=c23 -Wno-unused-parameter\
		 -I./inc/ -I./inc/external
CCFLAGS= -g -pg -pthread -fno-strict-aliasing -std=c23 -DDBG -fsanitize=address,undefined
CCFLAGS_PROD=-DPROD -Wno-format -DNOERRCHECK -march=native -ffast-math -ffp-contract=fast -fprefetch-loop-arrays
CCFLAGS_BENCH=-DPROD -Wno-format -DNOERRCHECK -DBENCH
EXEC_FILE=cablegen
BUILD=debug
PLATFORM=linux
LDFLAGS=-flto
FILES=$(addsuffix .o,$(addprefix build/,$(notdir $(basename $(wildcard src/*.c)))))
CCFLAGS_SHARED += -DVERSION=$$(git describe --tags --always --dirty)
.PHONY: all clean default gen_conf

ifeq ($(PLATFORM),windows)
CC=x86_64-w64-mingw32-gcc
LDFLAGS += -lm
endif
ifeq ($(PLATFORM),linux)
LDFLAGS += -lm -lc # I'm not sure why only clang needs an explicit -lc
endif

ifeq ($(BUILD),prod)
CCFLAGS = $(CCFLAGS_PROD)
endif
ifeq ($(BUILD),bench)
CCFLAGS = $(CCFLAGS_BENCH)
endif

ifeq (, $(shell which $(CC)))
$(error "No CC in $(PATH), consider changing CC in the makefile for your operating system")
endif

default: build/ini.o $(FILES) $(EXEC_FILE)
	@mkdir -p build/

all: build/ini.o $(FILES) $(EXEC_FILE)
	@mkdir -p build/
	@make gen_conf

gen_conf:
	@echo [gen_conf]
	@make -C gen_conf/ all PLATFORM=$(PLATFORM)

clean: 
	@rm -f build/*
	@make -C gen_conf/ clean

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
