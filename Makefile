#how to build windows version of raylib:
#make clean (to remove linux object files if there are any; WARNING: this will also delete libraylib.a)
#make  OS=Windows_NT CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar

#how to build linux version:
#make clean (WARNING: removes windows version)
#make

all:
	#build for linux
	clang++ -O3 -s -DNDEBUG -march=x86-64 *.cpp -o minesweeper -lraylib -ldl -pthread
	#build for windows
	x86_64-w64-mingw32-g++ -O3 -s -DNDEBUG *.cpp -o minesweeper.exe -static -I /home/enigma/programming/raylib/raylib-master/src -L /home/enigma/programming/raylib/raylib-master/src -lraylib -lopengl32 -lgdi32 -lwinmm
	ldd ./minesweeper #print dependencies of linux version (should be almost none)

