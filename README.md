# Cablegen
A high speed 2048 generator and solver in C

## Known issues:
- libwinpthread missing dll error on windows -- a copy is provided here, just drop it into the same directory or your PATH

## Usage (CLI frontend)

`cablegen [flags] [command]`

Example usage (solving [DPDF](https://wiki.2048verse.com/wiki/index.php/Double_Perimeter_Defense_Formation(4x4)) with a goal of 512 and 10 movable tiles)
```
./cablegen write initial 18ff07ff15ff1334 # sensible starting board. https://2048verse.com/p/18ff07ff15ff1334 
./cablegen write winstates 09ff00ff00ff0000 # a 512 on the top left -- 0s indicate a "don't care"
# edit cablegen.conf as fit
./cablegen --config=cablegen.conf generate
./cablegen --config=cablegen.conf solve
```

## Concepts

### Formation

A formation is a set of "unmergeable" tiles wherein neither the player nor the computer really has to consider
their movement in the general case, simplifying playing and solving. Some common examples: (an x denotes an unmergeable tile,
and a _ denotes a free space)
```
LL /  4422
_ _ _ _
_ _ _ _
x x _ _
x x _ _

DPDGap /  4411
_ _ _ _
_ _ _ _
x x x _
x x x _

2x4 / 4400
_ _ _ _
_ _ _ _
x x x x
x x x x

3x4 / 4440
_ _ _ _
_ _ _ _
_ _ _ _
x x x x
```

For a full list of common formations and notations thereof, see the [2048 formation dictionary](https://cdn.discordapp.com/attachments/1328460117783482399/1409709696549523637/2048_Formation_Dictionary_1.pdf)

A variant formation is one where the unmergeable tiles are treated as walls.

### Tile values

A tile is one nybble of a 64 bit hexadecimal, stored as an exponent, E.G. the board
```
2   0   2   2
256 128 32  8
x   x   x   8
x   x   x   16
```

is represented by the hex string `0x18ff07ff15ff1334`. Note that unmergeable tiles are represented by `0xf`, and empty tiles
by `0x0` (since 2^0 = 1 is not a tile in 2048).

### Pruning / Small tile sum limit

Some boards are so bad that they mustn't be fully solved for the computer to recognize they are such by a heuristic measure.
The way that cablegen handles this is via the `stsl`, `ltc`, and `smallest_large` variables. `stsl` stands for "Small tile
sum limit", and it encodes the largest tile sum by all non-large (i.e. small) tiles within a board, above which a board will be pruned.
`ltc` stands for "Large tile count" and denotes the most large tiles that are allowed on a board. Pruning may or may not increase speed,
depending on the aggressiveness, but will definitely impact accuracy and file size, given that any generated boards are actually pruned.

## Commands

`generate` -- Generate boards

`solve` -- Solve positions

`generate_solve` -- Generate, and then solve

`read [FILE]` -- lists all boards in FILE

`write [FILE] [BOARD] (BOARD) ...` -- writes one or more boards into FILE, for use as winstates or initial boards

`lookup [BOARD]` -- looks for BOARD

`explore [TABLE]` -- lists all solved boards in TABLE

`play [BOARD]` -- simulate an optimal game of BOARD with random spawns

## Flags

> Note that later flags override earlier flags, and config files will override the values that are set within them appropriately

| Short option | Long Option | Description | Datatype
|--|--|--|--|
|-C|--config             |Specifies a config to read flags from. If unspecified, first ~/.config/cablegen/cablegen.conf and then ./cablegen.conf will be read. If neither are found, default values will be used. |String|
|-f|--free-formation     |Specifies whether unmergable (0xf) tiles should be allowed to move (default: false) |Bool|
|-v|--ignore-unmergeable |treats unmergable tiles as walls (default: false)|Bool
|-c|--cores              |Chooses the amount of cores to use while generating or solving (default:1)|Int
|-n|--nox                |Treats a tile with this tile on it as dead (0 to disable) (default:0)|Int
|-b|--bdir               |Select a directory for where to put generated positions (default:./boards/)|String
|-t|--tdir               |Select a directory for where to put solved tables (default:./tables/)|String
|-i|--initial            |Select a file to start generation from (default:./initial)|String
|-e|--end-gen            |Choose the tile sum to end generation (default: 1200)| Int
|-E|--end-solve          |Choose the tile sum to end solving (default: 0)| Int
|-w|--winstate           |Select a file to consider winstates from (default: ./winstates)| String
|-d|--delete             |Delete positions after solving them (default: false) | Bool

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
; compress boards and positions while generating and solving (currently unused rn)
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
; large tile count for pruning -- this should AT MOST be the number of free spaces minus smallest_large
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

If you don't wish to do all the work of configuration, and just want a quick and dirty solution, check out the gen_conf folder

## Building

Cablegen is built using GNU Make. To build the cablegen object only, simply `make`. By default, the build mode is debug,
which relies on asan and ubsan. If you are not a developer and have no need for these, use one either `make BUILD=bench`,
or `make BUILD=prod`. 

Debug mode:
- Enables address sanitizer and ub sanitizer (I am aware that sort.h invokes UB, I'm working on it ;))
- Enables extra sanity checks on reading and writing
- Turns on error checking that other build types can live without

Production mode:
- Disables unnecessary error checks (and some necessary ones too, to be frank)
- Optimizes more aggressively
- Strips names

Benchmark mode:
- Doesn't strip names
- Writes a DOT file with the benchmark profiling to `bench/`

e (preprocessing only):
- Runs only the preprocessor. Useful for seeing what attributes are implicit and not much else


### Supported platforms

By default, cablegen builds for linux. To build for windows, pass `PLATFORM=windows` to `make`. I will not be adding mac
support unless a wealthy benefactor with a macintosh becomes interested in this project.

### Targets

`clean`: cleans `gen_conf` and `frontends` (along with cablegen's artifacts)

`all`: builds all of the below

`gen_conf`: builds `gen_conf`

`frontends`: builds all frontends (the frontend artifacts live in frontends/)

### Options

`BUILD`: `bench | prod | nosanitize | debug | e`

`LIBTYPE`: `static | dynamic`

`PLATFORM`: `windows | linux`


## Wishlist

- GUI
- Replay analysis
- Constant time unmasking
- Pruning while solving
- GPU solving
- Option for rational probabilities
- Branchless moving without a move4 LUT
- Compression
- In-RAM compression
- Proper symmetry generation for variants
- More customizable pruning
- Cross-compatible tablegen API 
- Proper unit testing
- Threshold for inaccuracies

## Contributing

Contributions are welcome, but here are some guidelines

- If a function is only used internally, declare it as static (or static inline if appropriate)
- Name functions in snake_case
- Name local variables however you'd like
- Keep the API as simple as possible
- Use globals in favor of static variables, and if they are only used internally, preface them with an underscore
- Try to keep all the tests passing :-)

## Note

A large part of this program structure and concepts are inspired by xp's [2048-trainer](https://github.com/1h0si/2048-trainer) and aminos'
[2048EndgameTablebase](https://github.com/game-difficulty/2048EndgameTablebase), along with its predecessor. Another similar program is 
[2048-tables](https://github.com/CubeyTheCube/2048-tables/tree/main) by cubey. Of these three, only 2048EndgameTablebase is in active
development, and as of right now, is more robust than cablegen.

## Used libraries

[cfgpath](https://github.com/Malvineous/cfgpath) -- Unlicense

[sort](https://github.com/swenson/sort) -- MIT License

[ini](https://github.com/rxi/ini/) -- MIT License

[lz4](https://github.com/lz4/lz4/) -- BSD 2-Clause License
