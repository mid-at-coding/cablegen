# Cablegen
A high speed 2048 generator and solver, entirely in C
## Known issues:
- libwinpthread missing dll error on windows -- a copy is provided here, just drop it into the same directory or your PATH
## Usage
`generate (CONFIG)` -- Generate boards, optionally specifiying an alternate config file

`solve (CONFIG)` -- Solve positions, optionally specifying an alternate config file

`read [FILE]` -- lists all boards in FILE

`write [FILE] [BOARD] (BOARD) ...` -- writes one or more boards into FILE, for use as winstates or initial boards

`lookup [BOARD] (TDIR)` -- looks for BOARD, optionally specifying an alternate directory to look in

`explore [TABLE]` -- lists all solved boards in TABLE

`play [BOARD] (TDIR)` -- simulate an optimal game of BOARD with random spawns

## Configuration

Configuration is stored in an INI file and defines many properties of a table

```
; example configuration
; global settings 
[Cablegen]
; whether unmergeable tiles (0xf) are allowed to move
free_formation = false 
; how many cores are used during generation and solving
cores = 1
; generate and solve as if merging to this tile will lead to death
nox = 0

; generation specific settings
[Generate]
: where generated unsolved boards will go, relative to where cablegen is run
dir = ./boards/
initial = ./initial
; where the initial boards for generation are
end = 800             
; the tile sum where generation stops
premove = false       
; move initial boards before spawning

[Solve]   
; solving specific settings
dir = ./tables/       
; where solved boards will go, relative to where cablegen is run
winstates = ./winstate
; where the winstates for solving are
end = 14              
; the lowest tile sum, where solving ends
score = false         
; use score instead of win percentage
```

## Note

A large part of this program structure and concepts are inspired by xp's [2048-trainer](https://github.com/1h0si/2048-trainer) and aminos'
[2048EndgameTablebase](https://github.com/game-difficulty/2048EndgameTablebase), along with its predecessor. Another similar program is 
[2048-tables](https://github.com/CubeyTheCube/2048-tables/tree/main) by cubey. Of these three, only 2048EndgameTablebase is in active
development, and as of right now, is more robust than cablegen.

## Used libraries

[C-Thread-Pool](https://github.com/Pithikos/C-Thread-Pool) -- MIT License

[cfgpath](https://github.com/Malvineous/cfgpath) -- Unlicense

[sort](https://github.com/swenson/sort) -- MIT License

[ini](https://github.com/rxi/ini/) -- MIT License
