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

// --- DOMES ---
typedef struct {
    int id;
    char title[250];
    double budget;
    double popularity;
    double runtime;
    char text_feature[600]; 
    unsigned int minhash_sig[NUM_HASHES];
} Movie;

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

// --- HELPER: Fix Decimal Comma ---
double parse_european_double(char *str) {
    if (!str) return 0.0;
    char temp[100];
    strncpy(temp, str, 99);
    temp[99] = '\0';
    for(int i=0; temp[i]; i++) {
        if(temp[i] == ',') temp[i] = '.';
    }
    return atof(temp);
}

// --- HELPER: Get Field (Safe Version) ---
// Πλέον αποθηκεύει το αποτέλεσμα στο output buffer που του δίνουμε
void get_csv_field(char *line, int index, char *output) {
    char *p = line;
    int current_idx = 0;
    int buf_pos = 0;
    output[0] = '\0'; // Αρχικοποίηση

    // Προσπέραση πεδίων
    while (current_idx < index && *p != '\0') {
        if (*p == ';') current_idx++;
        p++;
    }

    if (*p == '\0') return; 

    // Ανάγνωση πεδίου
    while (*p != '\0' && *p != ';' && buf_pos < 1023) {
        output[buf_pos++] = *p;
        p++;
    }
    output[buf_pos] = '\0';
}

int load_csv(const char *filename, Movie *movies) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("ERROR: Could not find file '%s'\n", filename);
        return 0;
    }

    char line[MAX_LINE];
    int count = 0;
    
    // Buffers για τα πεδία (ώστε να μην αλληλοκαλύπτονται)
    char s_title[1024], s_genres[1024], s_budget[1024], s_run[1024], s_pop[1024];

    printf("Loading data from %s...\n", filename);
    
    // Skip Header
    fgets(line, MAX_LINE, file);

    while (fgets(line, MAX_LINE, file) && count < MAX_MOVIES) {
        line[strcspn(line, "\r\n")] = 0; 

        // Mapping (Βάσει του Screenshot σου):
        // 1: Title, 6: Genres, 8: Budget, 10: Runtime, 11: Popularity
        get_csv_field(line, 1, s_title);
        get_csv_field(line, 6, s_genres);
        get_csv_field(line, 8, s_budget);
        get_csv_field(line, 10, s_run);
        get_csv_field(line, 11, s_pop);

        if (strlen(s_budget) > 0 && strlen(s_pop) > 0) {
            double b = parse_european_double(s_budget);
            double p = parse_european_double(s_pop);
            double r = parse_european_double(s_run);

            // Φιλτράρισμα: Κρατάμε ταινίες με υπαρκτό budget
            if (b > 100) { 
                movies[count].id = count;
                movies[count].budget = b;
                movies[count].popularity = p;
                movies[count].runtime = r;
                
                if (strlen(s_title) > 0) strncpy(movies[count].title, s_title, 249);
                else strcpy(movies[count].title, "Unknown");
                
                if (strlen(s_genres) > 0) strncpy(movies[count].text_feature, s_genres, 599);
                else strcpy(movies[count].text_feature, "");

                compute_minhash(&movies[count]);
                
                // Debug Print (Πρώτες 3 εγγραφές)
                if (count < 3) {
                    printf("Parsed [%d]: %s | B:%.0f P:%.2f R:%.0f\n", 
                           count, movies[count].title, b, p, r);
                }
                count++;
            }
        }
    }
    fclose(file);
    printf("Successfully loaded %d movies.\n", count);
    return count;
}

void generate_dummy_data(Movie *movies, int n) {}

#endif