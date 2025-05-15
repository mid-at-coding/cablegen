# Cablegen
A high speed 2048 generator and solver, entirely in C
## Known issues:
- Settings aren't fully supported and documented yet (but coming soon!)

- Generation is weirdly slow, especially on multiple threads
## Usage
`generate [FILE] [DIR] [END] [CORES]` -- interprets FILE as containing one or more boards, and then generates every sub-board in DIR (remember the trailing slash), until the tile sum reaches END, using CORES cores

`solve [BDIR] [TDIR] [START] [END] [WINSTATE] [CORES]` -- interprets WINSTATE as containing one or more winstates (0x0 means that the tile can be anything), and then solves backwards from START to END, using CORES cores

`write [FILE] [BOARD] (BOARD) ...` -- writes one or more boards into FILE, for use as winstates or initial boards

`lookup [TDIR] [BOARD]` -- looks for BOARD in  the tables in TDIR

`explore [TABLE]` -- lists all boards in TABLE

## Used libraries

[cthreadpool](https://github.com/neo2043/cthreadpool) -- MIT License

[cfgpath](https://github.com/Malvineous/cfgpath) -- Unlicense

[clay](https://github.com/nicbarker/clay) -- Zlib license

[sort](https://github.com/swenson/sort) -- MIT License
