CC=clang++
CCFLAGS= -Wall -g -O2 -pthread -Wpedantic -Wextra -Wno-unused-parameter -std=c++23
LDFLAGS= -flto
EXEC_FILE=gen_conf
FILES=$(addsuffix .o,$(addprefix build/,$(notdir $(basename $(wildcard src/*.cpp)))))
.PHONY: all clean

all: $(FILES) gen_conf

clean: 
	@rm build/*.o

build/%.o: src/%.cpp 
	$(CC) $< $(CCFLAGS) -c -o $@ $(LDFLAGS)

gen_conf: $(FILES)
	$(CC) $(wildcard build/*.o) $(CCFLAGS) -o gen_conf $(LDFLAGS)
