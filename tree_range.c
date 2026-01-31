#include "movies_common.h"

typedef struct RangeNode {
    Movie *movie; 
    struct RangeNode *left, *right;
    Movie **sorted_aux; 
    int size;
} RangeNode;

// RAM Calculation: Includes structural nodes + aux arrays
long get_range_memory(RangeNode *n) {
    if (!n) return 0;
    long size = sizeof(RangeNode);
    size += n->size * sizeof(Movie*); // Aux array size
    size += get_range_memory(n->left);
    size += get_range_memory(n->right);
    return size;
}

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

int cmp_dim0(const void *a, const void *b) { 
    double v1 = (*(Movie**)a)->values[0]; double v2 = (*(Movie**)b)->values[0];
    return (v1 > v2) - (v1 < v2);
}
int cmp_dim1(const void *a, const void *b) { 
    double v1 = (*(Movie**)a)->values[1]; double v2 = (*(Movie**)b)->values[1];
    return (v1 > v2) - (v1 < v2);
}

RangeNode* build_range(Movie **mptr, int n) {
    if (n <= 0) return NULL;
    qsort(mptr, n, sizeof(Movie*), cmp_dim0); 
    
    int mid = n / 2;
    RangeNode *node = malloc(sizeof(RangeNode));
    node->movie = mptr[mid];
    node->size = n;
    
    node->sorted_aux = malloc(n * sizeof(Movie*));
    memcpy(node->sorted_aux, mptr, n * sizeof(Movie*));
    qsort(node->sorted_aux, n, sizeof(Movie*), cmp_dim1);

    node->left = build_range(mptr, mid);
    node->right = build_range(mptr + mid + 1, n - mid - 1);
    return node;
}

void free_range(RangeNode *node) {
    if (!node) return;
    free_range(node->left);
    free_range(node->right);
    if (node->sorted_aux) free(node->sorted_aux);
    free(node);
}

void update_range(RangeNode **root, Movie *target, double new_pop) {
    printf(" [Update] Moved '%s' (Pop: %.2f -> %.2f)\n", target->title, target->values[1], new_pop);
    target->values[1] = new_pop;
}

void query_range(RangeNode *node, double min[], double max[], Movie **res, int *cnt) {
    if (!node) return;
    
    if (node->movie->values[0] >= min[0] && node->movie->values[0] <= max[0]) {
        Movie *m = node->movie;
        if (!m->is_deleted) {
            int match = 1;
            for(int k=0; k<K_DIMS; k++) {
                if (m->values[k] < min[k] || m->values[k] > max[k]) { match=0; break; }
            }
            if (match) res[(*cnt)++] = m;
        }
        query_range(node->left, min, max, res, cnt);
        query_range(node->right, min, max, res, cnt);
    } 
    else if (node->movie->values[0] > max[0]) {
        query_range(node->left, min, max, res, cnt);
    } 
    else { 
        query_range(node->right, min, max, res, cnt);
    }
}

int main() {
    Movie *data = malloc(MAX_MOVIES * sizeof(Movie));
    int total_n = load_csv("movies.csv", data);
    Movie **ptrs = malloc(total_n * sizeof(Movie*));
    Movie **results = malloc(total_n * sizeof(Movie*));
    
    // ΔΙΟΡΘΩΣΗ ΓΙΑ 2D
    double minv[K_DIMS], maxv[K_DIMS];
    minv[0] = 1000; maxv[0] = 50000; 
    if (K_DIMS > 1) { minv[1] = 2;    maxv[1] = 50; }
    if (K_DIMS > 2) { minv[2] = 60;   maxv[2] = 180; }
    for(int i=3; i<K_DIMS; i++) { minv[i] = -1e9; maxv[i] = 1e9; }

    printf("\n=== Range Tree (%d Dims) ===\n", K_DIMS);
    printf("--------------------------------------------------------------------------\n");
    printf("| Size         | Build (s) | Insert (s) | Query (s) | Memory (MB) |\n");
    printf("--------------------------------------------------------------------------\n");

    int step = 20000;
    for(int n = step; n <= total_n; n += step) {
        for(int i=0; i<n; i++) ptrs[i] = &data[i];

        clock_t start = clock();
        RangeNode *root = build_range(ptrs, n);
        double build_time = (double)(clock()-start)/CLOCKS_PER_SEC;

        double insert_time = 0.0000; 

        int count = 0;
        start = clock();
        query_range(root, minv, maxv, results, &count);
        double query_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        long bytes = get_range_memory(root);
        double mem_mb = bytes / (1024.0 * 1024.0);
        
        printf("| %-12d | %-9.4f | %-10.4f | %-9.4f | %-11.2f |\n", n, build_time, insert_time, query_time, mem_mb);
        free_range(root);
    }
    printf("--------------------------------------------------------------------------\n");

    // DEMO FULL
    for(int i=0; i<total_n; i++) ptrs[i] = &data[i];
    RangeNode *root = build_range(ptrs, total_n);
    int count = 0;
    query_range(root, minv, maxv, results, &count);
    printf("Query Found: %d movies\n", count);
    
    if (count > 0) {
        printf("\n[Delete Demo] Removing: '%s'\n", results[0]->title);
        results[0]->is_deleted = 1;
        
        printf("[Update Demo] Updating popularity...\n");
        if(count > 1) update_range(&root, results[1], results[1]->values[1] + 10.0);
        
        int c2 = 0;
        query_range(root, minv, maxv, results, &c2); 
        
        if(c2 > 0) run_knn(results[0], results, c2, 5);

        if (c2 > 0) {
            printf("\n[LSH Similarity - Banding] Target: %s\n", results[0]->title);
            printf("(Showing candidates that collide in at least 1 band)\n");
            int found_sim = 0;
            for(int i=1; i<c2 && i<1000; i++) { 
                if (check_lsh_bands(results[0], results[i])) {
                    double sim = jaccard_similarity(results[0], results[i]);
                    if (sim > 0.3) {
                        printf(" -> Candidate: %s (Jaccard: %.2f)\n", results[i]->title, sim);
                        found_sim++;
                    }
                }
            }
            if (found_sim == 0) printf("No similar text features found in query results.\n");
        }
    }

    free_range(root);
    free(data); free(ptrs); free(results);
    return 0;
}