# 4d-minesweeper
a 4D minesweeper game made with Raylib

## How to play
At the beginning, all cells are hidden. Click on a cell to step on it and reveal it.
If you step on a mine, you lose the game. Each non-mine cell will show how many of its
neighbors are mines, minus the number of flagged cells around it: a negative number means
you have flagged too many neighbors of the cell.
When you click on a cell of which none of the neighbors are mines (no number on it),
adjacent cells are revealed, and this process keeps going until all such cells have only
revealed neighbors.
You win by revealing every non-mine cell on the board.

## Display
The 4D playfield is split up into a 2D grid of 2D layers.
Each cell can have up to 80 neighbors. For visual help, the neighbors are highlighted.

## Adjusting resolution and windowed mode
To change the resolution or play in windowed mode, the resolution/window size must be specified in the terminal.
Examples:
`minesweeper 1920 1080` launches 4D Minesweeper in fullscreen, in a 1920x1080 resolution.
`minesweeper 640 480 --windowed` launches 4D Minesweeper in windowed mode, in a 640x480 resolution.
If you forget this, type `minesweeper -h` or `minesweeper --help` (both work).

## Controls
### Menu controls:
ESC to quit the game
Hover over an input box to change its value
You can start the game only when grid size is at least 4, and there is at least 1 mine
### Game controls:
Arrow keys or mouse drag to move camera (make sure not to accidentally click on tiles!)
L/K or mouse wheel to zoom
Reveal a tile with left click, flag with right click
When game over, press SPACE to restart

## Credits
This game was made with Raylib. It was inspired by the Steam game "4D Minesweeper".
Sadly, that game isn't implemented in the most efficient way possible. This version
allows up to a 16⁴ grid, and didn't lag or freeze except when displaying an unplayable
amount of mines.
