# Cablegen
A high speed 2048 generator and solver, entirely in C
## Known issues:

## Usage
`generate (CONFIG)` -- Generate boards, optionally specifiying an alternate config file

`solve (CONFIG)` -- Solve positions, optionally specifying an alternate config file

`read [FILE]` -- lists all boards in FILE

`write [FILE] [BOARD] (BOARD) ...` -- writes one or more boards into FILE, for use as winstates or initial boards

`lookup [BOARD] (TDIR)` -- looks for BOARD, optionally specifying an alternate directory to look in

`explore [TABLE]` -- lists all solved boards in TABLE

## Configuration

Configuration is stored in an INI file and defines many properties of a table

```
; example configuration
[Cablegen] ; global settings 
free_formation = false ; whether unmergeable tiles (0xf) are allowed to move
cores = 1              ; how many cores are used during generation and solving

[Generate] ; generation specific settings
dir = ./boards/        : where generated unsolved boards will go, relative to where cablegen is run
initial = ./initial    ; where the initial boards for generation are
end = 800              ; the tile sum where generation stops

[Solve]    ; solving specific settings
dir = ./tables/        ; where solved boards will go, relative to where cablegen is run
winstates = ./winstate ; where the winstates for solving are
end = 14               ; the lowest tile sum, where solving ends
```

## Used libraries

[cthreadpool](https://github.com/neo2043/cthreadpool) -- MIT License

[cfgpath](https://github.com/Malvineous/cfgpath) -- Unlicense

[clay](https://github.com/nicbarker/clay) -- Zlib license

[sort](https://github.com/swenson/sort) -- MIT License

[ini](https://github.com/rxi/ini/) -- MIT License
