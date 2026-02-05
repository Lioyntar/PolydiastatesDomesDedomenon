
To build and run this project, you need:
1.  GCC Compiler.
2.  Make tool.



Project Structure
main_menu.c: The entry point for the C program.

tree_*.c: Source code for each data structure (k-d, Quad, Range, R-Tree).

movies_common.h: Shared structures and helper functions.

movies.csv: The dataset file (Required).

makefile: Automation script for compiling the C code.


Setup & Usage Instructions

Download the dataset from Kaggle: https://www.kaggle.com/datasets/mustafasayed1181/movies-metadata-cleaned-dataset-19002025

Rename the main file to movies.csv.

Place movies.csv inside the same folder as the .c files.



Running the Data Structures
make
main_menu.exe
make clean

To change the dimension you need to change the K_DIMS value on the movies_common.h file.


