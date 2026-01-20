# Multi-Dimensional Data Structures & Data Analysis Project

This project implements and evaluates advanced multi-dimensional data structures in **C** for indexing movie metadata. Additionally, it includes a **Python** module for Exploratory Data Analysis (EDA) to generate visual insights for the project report.

## 🚀 Key Features

### C Implementation (Data Structures)
* **4 Data Structures:**
    1.  **k-d Tree:** Partitioning based on median values of dimensions.
    2.  **Quad Tree (Octree):** Spatial indexing for 3D data (Budget, Popularity, Runtime).
    3.  **Range Tree:** Multi-level tree structure for fast range queries.
    4.  **R-Tree:** Object grouping using Minimum Bounding Rectangles (MBRs).
* **Performance Evaluation:** Measures Build Time, Dynamic Insert Time, and Query Time for datasets up to 200k records.
* **kNN Search:** Finds the *k* nearest neighbors based on Euclidean distance.
* **LSH (Locality Sensitive Hashing):** Implements MinHash with Banding technique to find semantically similar movies based on **Genres** and **Production Companies**.

### Python Implementation (Data Analysis)
* **Automated EDA:** A script (`analysis_full.py`) that handles CSV parsing (including separator/encoding issues).
* **Visualizations:** Automatically generates 5 key graphs for the project report:
    * Movie Runtime Distribution.
    * Budget vs. Popularity Correlation.
    * Release Year Distribution.
    * Top 10 Genres (Textual analysis for LSH).
    * Top 10 Production Companies (Textual analysis for LSH).

---

## 🛠️ Prerequisites

To build and run this project, you need:

1.  **GCC Compiler** (MinGW for Windows or standard GCC for Linux/Mac).
2.  **Make** tool.
3.  **Python 3.x** (for the analysis part).

Install the required Python libraries:
```bash
pip install pandas matplotlib seaborn




Project Structure
main_menu.c: The entry point for the C program.

tree_*.c: Source code for each data structure (k-d, Quad, Range, R-Tree).

movies_common.h: Shared structures and helper functions.

analysis_full.py: Python script for generating graphs.

movies.csv: The dataset file (Required).

makefile: Automation script for compiling the C code.


Setup & Usage Instructions
1. Dataset Setup (CRUCIAL)
The program requires the Movies Metadata Cleaned Dataset to run.

Download the dataset from Kaggle: https://www.kaggle.com/datasets/mustafasayed1181/movies-metadata-cleaned-dataset-19002025

Rename the main file to movies.csv.

Place movies.csv inside the same folder as the .c and .py files.




Running the Data Structures (C)
make
main_menu.exe
make clean

Run the analysis:
go to analysis_notebook.ipynb
Run the code by pressing the triangle play button at the left of the code block.
