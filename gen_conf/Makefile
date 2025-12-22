CC=clang++
CCFLAGS= -Wall -g -O2 -pthread -Wextra -Wno-unused-parameter -std=c++23
LDFLAGS= -flto
EXEC_FILE=gen_conf
PLATFORM=linux
FILES=$(addsuffix .o,$(addprefix build/,$(notdir $(basename $(wildcard src/*.cpp)))))
.PHONY: all clean

ifeq ($(PLATFORM),windows)
CC=x86_64-w64-mingw32-g++
endif
ifeq ($(PLATFORM),linux)
CC=clang++
endif
ifeq (, $(shell which $(CC)))
$(error "No CC in $(PATH), consider changing CC in the makefile for your operating system")
endif

all: $(FILES) gen_conf
	@mkdir -p build

clean: 
	@rm -f build/*.o

ifeq ($(PLATFORM), linux)
build/%.o: src/%.cpp 
	@echo [CC++] $@
	@$(CC) $< $(CCFLAGS) -c -o $@ $(LDFLAGS)
else ifeq ($(PLATFORM), windows)
build/%.o: src/%.cpp 
	# Windows isn't supported. If you know how to get the return status of a subshell, you can modify the code.
	# but, I am not too interested in spending time maintaining windows-specific code for my linux machine ;)
	$(error Only linux is supported!)
endif

gen_conf: $(FILES)
	@echo [CC++] $@
	@$(CC) $(wildcard build/*.o) $(CCFLAGS) -o gen_conf $(LDFLAGS)
