all:
	clang++ -O3 -Ofast -flto -s -DNDEBUG -march=x86-64 *.cpp -lraylib
