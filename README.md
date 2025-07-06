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

Configuration is stored in an INI file and defines many properties of a table. Cablegen will try to read first from `~/.config/cablegen.conf`(on linux) or `C:\Users\[name]\AppData\Roaming\cablegen.ini`(on windows), and then from `cablegen.conf` in the directory it is run.

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
; treat 0xf tiles as if they were walls -- free formation must be true for this setting to have an effect (generally only used for variants)
ignore_f = false
; compress boards and positions while generating and solving
compress = true

; generation specific settings
[Generate]
: where generated unsolved boards will go, relative to where cablegen is run
dir = ./boards/
; where the initial boards for generation are
initial = ./initial
; the tile sum where generation stops
end = 800             
; move initial boards before spawning
premove = false       
; prune unlikely boards
prune = true
; small tile sum limit for pruning
stsl = 58
; large tile count for pruning
ltc = 5
; defines the smallest large tile, below which a tile is "small"
smallest_large = 6

; solving specific settings
[Solve]   
; where solved boards will go, relative to where cablegen is run
dir = ./tables/       
; where the winstates for solving are
winstates = ./winstate
; the lowest tile sum, where solving ends
end = 14              
; use score instead of win percentage
score = false         
; delete board files while solving to save space
delete_boards = false

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

[lz4](https://github.com/lz4/lz4/) -- BSD 2-Clause License
