#ifndef MOVIES_COMMON_H
#define MOVIES_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#define MAX_LINE 8192
#define MAX_MOVIES 200000 
#define NUM_HASHES 20     

// --- ΡΥΘΜΙΣΗ ΔΙΑΣΤΑΣΕΩΝ ---
// Αλλάξτε το σε 2, 3, 4 ή 5
#define K_DIMS 5 

// --- DOMES ---
typedef struct {
    int id;
    char title[250];
    // values[0]: Budget
    // values[1]: Popularity
    // values[2]: Runtime (if K>2)
    // values[3]: Vote Average (if K>3)
    // values[4]: Revenue (if K>4)
    double values[K_DIMS]; 

    char text_feature[600]; 
    unsigned int minhash_sig[NUM_HASHES];
    int is_deleted; 
} Movie;

typedef struct {
    Movie *movie;
    double dist;
} Neighbor;

// --- MINHASH FUNCTIONS ---
unsigned int hash_str(const char *str, int seed) {
    unsigned int hash = 5381 + seed;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

void compute_minhash(Movie *m) {
    char temp[600];
    strncpy(temp, m->text_feature, 599);
    temp[599] = '\0';
    for(int i=0; i<NUM_HASHES; i++) m->minhash_sig[i] = 0xFFFFFFFF;
    
    char *token = strtok(temp, " ,.-|:;'[]\"");
    while (token != NULL) {
        if (strlen(token) > 2) { 
            for (int i = 0; i < NUM_HASHES; i++) {
                unsigned int h = hash_str(token, i * 98765); 
                if (h < m->minhash_sig[i]) m->minhash_sig[i] = h;
            }
        }
        token = strtok(NULL, " ,.-|:;'[]\"");
    }
}

double jaccard_similarity(Movie *m1, Movie *m2) {
    int matches = 0;
    for (int i = 0; i < NUM_HASHES; i++) {
        if (m1->minhash_sig[i] == m2->minhash_sig[i]) matches++;
    }
    return (double)matches / NUM_HASHES;
}

// --- kNN & DISTANCE FUNCTIONS ---
double euclidean_dist(Movie *m1, Movie *m2) {
    double sum = 0.0;
    for (int i = 0; i < K_DIMS; i++) {
        double diff = m1->values[i] - m2->values[i];
        if (i == 0 || (K_DIMS > 4 && i == 4)) diff /= 1000000.0; // Scaling for large numbers
        sum += diff * diff;
    }
    return sqrt(sum);
}

int cmp_neighbor(const void *a, const void *b) {
    double d1 = ((Neighbor*)a)->dist;
    double d2 = ((Neighbor*)b)->dist;
    return (d1 > d2) - (d1 < d2);
}

void run_knn(Movie *target, Movie **candidates, int count, int k) {
    if (count == 0) return;
    Neighbor *neighbors = malloc(count * sizeof(Neighbor));
    
    for(int i=0; i<count; i++) {
        neighbors[i].movie = candidates[i];
        neighbors[i].dist = euclidean_dist(target, candidates[i]);
    }
    
    qsort(neighbors, count, sizeof(Neighbor), cmp_neighbor);
    
    printf("\n[kNN Search] Top %d Nearest Neighbors (Numeric Space) for '%s':\n", k, target->title);
    for(int i=0; i<k && i<count; i++) {
        if (neighbors[i].dist >= 0) { 
            printf(" %d. %s (Dist: %.2f)\n", i+1, neighbors[i].movie->title, neighbors[i].dist);
        }
    }
    free(neighbors);
}

// --- CSV LOADING ---
double parse_european_double(char *str) {
    if (!str) return 0.0;
    char temp[100];
    strncpy(temp, str, 99);
    temp[99] = '\0';
    for(int i=0; temp[i]; i++) if(temp[i] == ',') temp[i] = '.';
    return atof(temp);
}

void get_csv_field(char *line, int index, char *output) {
    char *ptr = line;
    int cur_col = 0;
    output[0] = '\0';
    while (*ptr && cur_col < index) {
        if (*ptr == ';' || *ptr == ',') cur_col++;
        ptr++;
    }
    if (*ptr == '\0' || *ptr == '\n' || *ptr == '\r') return;
    int i = 0;
    while (*ptr && *ptr != ';' && *ptr != ',' && *ptr != '\n' && *ptr != '\r') {
        output[i++] = *ptr++;
    }
    output[i] = '\0';
}

int load_csv(const char *filename, Movie *movies) {
    FILE *file = fopen(filename, "r");
    if (!file) { printf("ERROR: File %s not found.\n", filename); return 0; }
    char line[MAX_LINE];
    int count = 0;
    char s_title[1024], s_genres[1024];
    char s_vals[K_DIMS][100]; 
    
    printf("Loading data for %d dimensions...\n", K_DIMS);
    fgets(line, MAX_LINE, file); // Skip Header

    while (fgets(line, MAX_LINE, file) && count < MAX_MOVIES) {
        line[strcspn(line, "\r\n")] = 0; 
        
        get_csv_field(line, 1, s_title);
        get_csv_field(line, 6, s_genres);
        get_csv_field(line, 8, s_vals[0]);  // Budget
        get_csv_field(line, 11, s_vals[1]); // Popularity
        
        // Safe loading based on dimensions
        if (K_DIMS > 2) get_csv_field(line, 10, s_vals[2]); // Runtime
        if (K_DIMS > 3) get_csv_field(line, 12, s_vals[3]); // Vote Avg
        if (K_DIMS > 4) get_csv_field(line, 9, s_vals[4]);  // Revenue

        if (strlen(s_vals[0]) > 0) {
            double b = parse_european_double(s_vals[0]);
            if (b > 100 || b == 0) { 
                movies[count].id = count;
                movies[count].is_deleted = 0;
                
                movies[count].values[0] = b;
                movies[count].values[1] = parse_european_double(s_vals[1]);
                
                // ΔΙΟΡΘΩΣΗ: Έλεγχος πριν γράψουμε στη μνήμη
                if (K_DIMS > 2) movies[count].values[2] = parse_european_double(s_vals[2]);
                if (K_DIMS > 3) movies[count].values[3] = parse_european_double(s_vals[3]);
                if (K_DIMS > 4) movies[count].values[4] = parse_european_double(s_vals[4]);

                // Initialize rest to 0 safely
                for(int k=(K_DIMS < 2 ? 2 : K_DIMS); k<K_DIMS; k++) movies[count].values[k] = 0.0;

                if (strlen(s_title) > 0) strncpy(movies[count].title, s_title, 249);
                else strcpy(movies[count].title, "Unknown");
                
                if (strlen(s_genres) > 0) strncpy(movies[count].text_feature, s_genres, 599);
                else strcpy(movies[count].text_feature, "");

                compute_minhash(&movies[count]);
                count++;
            }
        }
    }
    fclose(file);
    printf("Loaded %d movies.\n", count);
    return count;
}
#endif