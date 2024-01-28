all:
	clang++ -O3 -flto -s -DNDEBUG -march=x86-64 *.cpp -lraylib -ldl -pthread
	ldd ./a.out #print dependencies (should be almost none)
