# PolydiastatesDomesDedomenon

# Multi-Dimensional Data Structures Project (C Implementation)






1\.  ''k-d Tree''

2\.  ''Quad Tree'' 

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

1\.  ''GCC Compiler'' 

2\.  ''Make'' tool.



\## Setup Instructions



\### 1. Dataset Setup

Crucial Step: The program requires the movie dataset to run.

1\.  Ensure you have the file ''`movies.csv`''.(https://www.kaggle.com/datasets/mustafasayed1181/movies-metadata-cleaned-dataset-19002025)

2\.  Place `movies.csv` inside the same folder as the .c source files.




\### 2. Compilation

A `makefile` is provided to automate the build process.



terminal:

make
main_menu.exe (to run the trees)
make clean

