#include "movies_common.h"

typedef struct RangeNode {
    Movie *movie; 
    struct RangeNode *left, *right;
    Movie **sorted_y; 
    int size;
} RangeNode;

int cmp_b(const void *a, const void *b) { 
    double v1 = (*(Movie**)a)->budget; double v2 = (*(Movie**)b)->budget;
    return (v1 > v2) - (v1 < v2);
}
int cmp_p(const void *a, const void *b) { 
    double v1 = (*(Movie**)a)->popularity; double v2 = (*(Movie**)b)->popularity;
    return (v1 > v2) - (v1 < v2);
}

// Bulk Build
RangeNode* build_range(Movie **mptr, int n) {
    if (n <= 0) return NULL;
    qsort(mptr, n, sizeof(Movie*), cmp_b); 
    
    int mid = n / 2;
    RangeNode *node = malloc(sizeof(RangeNode));
    node->movie = mptr[mid];
    node->size = n;
    
    node->sorted_y = malloc(n * sizeof(Movie*));
    memcpy(node->sorted_y, mptr, n * sizeof(Movie*));
    qsort(node->sorted_y, n, sizeof(Movie*), cmp_p);

    node->left = build_range(mptr, mid);
    node->right = build_range(mptr + mid + 1, n - mid - 1);
    return node;
}

// --- MEMORY DEALLOCATION (NEW) ---
void free_range(RangeNode *node) {
    if (!node) return;
    free_range(node->left);
    free_range(node->right);
    if (node->sorted_y) free(node->sorted_y); // Crucial!
    free(node);
}

// --- DYNAMIC INSERT ---
void insert_range(RangeNode **root, Movie *m) {
    if (!*root) {
        RangeNode *node = malloc(sizeof(RangeNode));
        node->movie = m;
        node->left = node->right = NULL;
        node->size = 1;
        node->sorted_y = malloc(sizeof(Movie*));
        node->sorted_y[0] = m;
        *root = node;
        return;
    }
    RangeNode *node = *root;
    node->size++;
    node->sorted_y = realloc(node->sorted_y, node->size * sizeof(Movie*));
    node->sorted_y[node->size - 1] = m;
    // Insert Sort optimization
    for (int i = node->size - 1; i > 0; i--) {
        if (node->sorted_y[i]->popularity < node->sorted_y[i-1]->popularity) {
            Movie *temp = node->sorted_y[i];
            node->sorted_y[i] = node->sorted_y[i-1];
            node->sorted_y[i-1] = temp;
        } else break;
    }
    if (m->budget < node->movie->budget) insert_range(&(node->left), m);
    else insert_range(&(node->right), m);
}

// --- STRUCTURAL UPDATE ---
void update_range(RangeNode **root, Movie *target, double new_pop) {
    target->is_deleted = 1;
    Movie *new_m = malloc(sizeof(Movie));
    *new_m = *target;
    new_m->popularity = new_pop;
    new_m->is_deleted = 0;
    insert_range(root, new_m);
    printf(" [Structural Update] Inserted new version of '%s'\n", target->title);
}

void query_range(RangeNode *node, double min[], double max[], Movie **res, int *cnt) {
    if (!node) return;
    if (!node->movie->is_deleted &&
        node->movie->budget >= min[0] && node->movie->budget <= max[0] &&
        node->movie->popularity >= min[1] && node->movie->popularity <= max[1] &&
        node->movie->runtime >= min[2] && node->movie->runtime <= max[2]) {
        res[(*cnt)++] = node->movie;
    }
    if (node->movie->budget >= min[0]) query_range(node->left, min, max, res, cnt);
    if (node->movie->budget <= max[0]) query_range(node->right, min, max, res, cnt);
}

// --- LSH BANDING ---
int check_lsh_bands(Movie *m1, Movie *m2) {
    int bands = 5, rows = 4;
    for (int b = 0; b < bands; b++) {
        int match = 1;
        for (int r = 0; r < rows; r++) {
            if (m1->minhash_sig[b * rows + r] != m2->minhash_sig[b * rows + r]) { match = 0; break; }
        }
        if (match) return 1;
    }
    return 0;
}

int main() {
    Movie *data = malloc(MAX_MOVIES * sizeof(Movie));
    int total_n = load_csv("movies.csv", data);
    Movie **ptrs = malloc(total_n * sizeof(Movie*));
    Movie **results = malloc(total_n * sizeof(Movie*));
    
    double minv[] = {1000, 2, 60};     
    double maxv[] = {50000, 50, 180};  

    printf("\n=== EXPERIMENTAL EVALUATION (Scalability) ===\n");
    printf("| Dataset Size | Build (s) | Insert (ms)| Query (s) |\n");
    printf("|--------------|-----------|------------|-----------|\n");

    for (int n = 50000; n <= 200000; n += 50000) {
        if (n > total_n) break;
        for(int i=0; i<n; i++) ptrs[i] = &data[i];
        
        clock_t start = clock();
        RangeNode *root = build_range(ptrs, n);
        double build_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        start = clock();
        for(int k=0; k<50; k++) insert_range(&root, &data[rand()%n]);
        double insert_time = ((double)(clock()-start)/CLOCKS_PER_SEC) * 1000.0 / 50.0;

        int count = 0;
        start = clock();
        query_range(root, minv, maxv, results, &count);
        double query_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        printf("| %-12d | %-9.4f | %-10.4f | %-9.4f |\n", n, build_time, insert_time, query_time);
        free_range(root);
    }
    
    // Full Demo
    for(int i=0; i<total_n; i++) ptrs[i] = &data[i];
    RangeNode *root = build_range(ptrs, total_n);
    int count = 0;
    query_range(root, minv, maxv, results, &count);
    
    if (count > 0) {
        printf("\n[Delete Demo] Removing: '%s'\n", results[0]->title);
        printf("\n[Update Demo] Updating popularity...\n");
        update_range(&root, results[0], results[0]->popularity + 10.0);
        
        int c2 = 0;
        query_range(root, minv, maxv, results, &c2); 
        run_knn(results[0], results, c2, 5);

        // --- LSH ---
        printf("\n[LSH Similarity - Banding] Target: %s\n", results[0]->title);
        int found_sim = 0;
        for(int i=1; i<c2 && i<1000; i++) { 
            if (check_lsh_bands(results[0], results[i])) {
                double sim = jaccard_similarity(results[0], results[i]);
                printf(" -> [Bucket Match] %s (Jaccard: %.2f)\n", results[i]->title, sim);
                found_sim++;
                if(found_sim >= 5) break; 
            }
        }
    }
    free_range(root);
    free(data);
    free(ptrs);
    free(results);
    return 0;
}