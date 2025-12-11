# PolydiastatesDomesDedomenon

DATA

# Multi-Dimensional Data Structures Project (C Implementation)



This project implements and experimentally evaluates four multi-dimensional data structures for indexing and querying movie metadata.



\## implemented Data Structures

1\.  ''k-d Tree''

2\.  ''Quad Tree'' (Adapted for 3D data: Budget, Popularity, Runtime)

3\.  ''Range Tree''

4\.  ''R-Tree''



\## Features

\- ''Scalability Evaluation:'' Measures Build Time, Dynamic Insertion Time, and Query Time for datasets ranging from 50k to 200k records.

\- ''Dynamic Operations:'' Supports Insert, Logical Delete, and Structural Updates.

\- ''kNN Search:'' Finds the $k$ nearest neighbors based on Euclidean distance (Budget, Popularity, Runtime).

\- ''LSH (Locality Sensitive Hashing):'' Implements MinHash with Banding to find movies with similar textual features (Genres).



---



\## Prerequisites



To build and run this project, you need:

1\.  ''GCC Compiler'' (e.g., MinGW for Windows).

2\.  ''Make'' tool.



\## Setup Instructions



\### 1. Dataset Setup

Crucial Step: The program requires the movie dataset to run.

1\.  Ensure you have the file ''`movies.csv`''.

2\.  Place `movies.csv` inside the same folder as the .c source files.

&nbsp;  



\### 2. Compilation

A `makefile` is provided to automate the build process.



terminal:

make
main_menu.exe (to run the trees)
make clean

