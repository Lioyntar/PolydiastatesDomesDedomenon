#include "movies_common.h"

typedef struct KDNode {
    Movie *movie;
    struct KDNode *left, *right;
    int axis;
} KDNode;

// Comparators
int cmp_b(const void *a, const void *b) { return (*(Movie**)a)->budget > (*(Movie**)b)->budget ? 1 : -1; }
int cmp_p(const void *a, const void *b) { return (*(Movie**)a)->popularity > (*(Movie**)b)->popularity ? 1 : -1; }
int cmp_r(const void *a, const void *b) { return (*(Movie**)a)->runtime > (*(Movie**)b)->runtime ? 1 : -1; }

// --- BULK BUILD ---
KDNode* build_kdtree(Movie **mptr, int n, int depth) {
    if (n <= 0) return NULL;
    int axis = depth % 3;
    if (axis == 0) qsort(mptr, n, sizeof(Movie*), cmp_b);
    else if (axis == 1) qsort(mptr, n, sizeof(Movie*), cmp_p);
    else qsort(mptr, n, sizeof(Movie*), cmp_r);

    int mid = n / 2;
    KDNode *node = malloc(sizeof(KDNode));
    node->movie = mptr[mid];
    node->axis = axis;
    node->left = build_kdtree(mptr, mid, depth + 1);
    node->right = build_kdtree(mptr + mid + 1, n - mid - 1, depth + 1);
    return node;
}

// --- DYNAMIC INSERT ---
KDNode* insert_kdtree(KDNode *node, Movie *m, int depth) {
    if (!node) {
        KDNode *n = malloc(sizeof(KDNode));
        n->movie = m;
        n->axis = depth % 3;
        n->left = n->right = NULL;
        return n;
    }
    int axis = node->axis;
    double val = (axis == 0) ? m->budget : (axis == 1) ? m->popularity : m->runtime;
    double node_val = (axis == 0) ? node->movie->budget : (axis == 1) ? node->movie->popularity : node->movie->runtime;

    if (val < node_val) node->left = insert_kdtree(node->left, m, depth + 1);
    else node->right = insert_kdtree(node->right, m, depth + 1);
    return node;
}

// --- MEMORY DEALLOCATION (NEW) ---
void free_kdtree(KDNode *node) {
    if (!node) return;
    free_kdtree(node->left);
    free_kdtree(node->right);
    free(node);
}

// --- STRUCTURAL UPDATE ---
void update_kdtree(KDNode **root, Movie *target, double new_pop) {
    target->is_deleted = 1;
    Movie *new_m = malloc(sizeof(Movie));
    *new_m = *target; 
    new_m->popularity = new_pop;
    new_m->is_deleted = 0;
    *root = insert_kdtree(*root, new_m, 0);
    printf(" [Structural Update] Moved '%s' to new position (Pop: %.2f -> %.2f)\n", target->title, target->popularity, new_pop);
}

void query_kdtree(KDNode *node, double min[], double max[], Movie **res, int *cnt) {
    if (!node) return;
    Movie *m = node->movie;
    
    if (!m->is_deleted && 
        m->budget >= min[0] && m->budget <= max[0] &&
        m->popularity >= min[1] && m->popularity <= max[1] &&
        m->runtime >= min[2] && m->runtime <= max[2]) {
        res[(*cnt)++] = m;
    }
    double val = (node->axis == 0) ? m->budget : (node->axis == 1) ? m->popularity : m->runtime;
    if (val >= min[node->axis]) query_kdtree(node->left, min, max, res, cnt);
    if (val <= max[node->axis]) query_kdtree(node->right, min, max, res, cnt);
}

// --- LSH BANDING CHECK (NEW) ---
// Checks if at least one band matches (Candidate Selection)
int check_lsh_bands(Movie *m1, Movie *m2) {
    int bands = 5;
    int rows = 4; // 5 * 4 = 20 hashes
    for (int b = 0; b < bands; b++) {
        int match = 1;
        for (int r = 0; r < rows; r++) {
            if (m1->minhash_sig[b * rows + r] != m2->minhash_sig[b * rows + r]) {
                match = 0; break;
            }
        }
        if (match) return 1; // Collision in Bucket 'b'
    }
    return 0;
}

int main() {
    Movie *data = malloc(MAX_MOVIES * sizeof(Movie));
    int total_n = load_csv("movies.csv", data);
    
    Movie **ptrs = malloc(total_n * sizeof(Movie*));
    Movie **results = malloc(total_n * sizeof(Movie*));
    
    printf("\n=== EXPERIMENTAL EVALUATION (Scalability) ===\n");
    printf("| Dataset Size | Build (s) | Insert (ms)| Query (s) |\n");
    printf("|--------------|-----------|------------|-----------|\n");
    
    double minv[] = {1000, 2, 60};     
    double maxv[] = {50000, 50, 180}; 

    for (int n = 50000; n <= 200000; n += 50000) {
        if (n > total_n) break;
        for(int i=0; i<n; i++) ptrs[i] = &data[i];
        
        clock_t start = clock();
        KDNode *root = build_kdtree(ptrs, n, 0);
        double build_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        start = clock();
        for(int k=0; k<100; k++) insert_kdtree(root, &data[rand()%n], 0);
        double insert_time_ms = ((double)(clock()-start)/CLOCKS_PER_SEC) * 1000.0 / 100.0;

        int count = 0;
        start = clock();
        query_kdtree(root, minv, maxv, results, &count);
        double query_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        printf("| %-12d | %-9.4f | %-10.4f | %-9.4f |\n", n, build_time, insert_time_ms, query_time);
        
        free_kdtree(root); // Free memory after experiment
    }

    // --- FULL RUN ---
    printf("\n=== FULL DATASET OPERATIONS ===\n");
    for(int i=0; i<total_n; i++) ptrs[i] = &data[i];
    KDNode *root = build_kdtree(ptrs, total_n, 0);
    
    int count = 0;
    query_kdtree(root, minv, maxv, results, &count);
    
    if (count > 0) {
        printf("\n[Delete Demo] Removing: '%s'\n", results[0]->title);
        results[0]->is_deleted = 1;
        
        printf("\n[Update Demo] Updating popularity...\n");
        update_kdtree(&root, results[0], results[0]->popularity + 15.0);
        
        int c2 = 0;
        query_kdtree(root, minv, maxv, results, &c2);
        
        // --- kNN ---
        run_knn(results[0], results, c2, 5);
        
        // --- LSH with BANDING ---
        printf("\n[LSH Similarity - Banding Technique] Target: %s\n", results[0]->title);
        printf("(Showing candidates that collide in at least 1 band)\n");
        int found_sim = 0;
        for(int i=1; i<c2 && i<1000; i++) { 
            // 1. Banding Filter (Bucket Check)
            if (check_lsh_bands(results[0], results[i])) {
                // 2. Exact Jaccard Calculation
                double sim = jaccard_similarity(results[0], results[i]);
                printf(" -> [Bucket Match] %s (Jaccard: %.2f)\n", results[i]->title, sim);
                found_sim++;
                if(found_sim >= 5) break; 
            }
        }
        if (found_sim == 0) printf(" No candidates found in same LSH buckets.\n");
    }
    
    free_kdtree(root); // Final Cleanup
    free(data);
    free(ptrs);
    free(results);
    return 0;
}