all:
	gcc -I C:/ProjetsCode/Chip-8/src/include -L C:/ProjetsCode/Chip-8/src/lib main.c -lmingw32 -lSDL2main -lSDL2 -mwindows -o main 