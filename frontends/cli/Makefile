CC=clang
TABLEGEN=../../cablegen.so
CCFLAGS=-I../../api/
FILES=main.c
CCFLAGS_DBG=-lasan -lubsan

ifeq ($(PLATFORM),windows)
CC=x86_64-w64-mingw32-gcc
endif
ifeq ($(PLATFORM),linux)
CC=clang
endif

ifeq ($(BUILD),debug)
CCFLAGS += $(CCFLAGS_DBG)
endif


ifeq (, $(shell which $(CC)))
$(error "No CC in $(PATH), consider changing CC in the makefile for your operating system")
endif

.PHONY: all

all: main.c
	@echo [CC] CLI
	@$(CC) main.c parse.c -o cli $(TABLEGEN) $(CCFLAGS)

clean:
	@rm -f cli
