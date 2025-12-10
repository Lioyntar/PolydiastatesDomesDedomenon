CC = gcc
CFLAGS = -O3 -Wall
OBJ = main_menu.o

all: tree_kdtree.exe tree_quad.exe tree_range.exe tree_rtree.exe main_menu.exe

tree_kdtree.exe: tree_kdtree.c movies_common.h
	$(CC) $(CFLAGS) -o tree_kdtree.exe tree_kdtree.c

tree_quad.exe: tree_quad.c movies_common.h
	$(CC) $(CFLAGS) -o tree_quad.exe tree_quad.c

tree_range.exe: tree_range.c movies_common.h
	$(CC) $(CFLAGS) -o tree_range.exe tree_range.c

tree_rtree.exe: tree_rtree.c movies_common.h
	$(CC) $(CFLAGS) -o tree_rtree.exe tree_rtree.c

main_menu.exe: main_menu.c
	$(CC) $(CFLAGS) -o main_menu.exe main_menu.c

clean:
	rm -f *.exe *.o