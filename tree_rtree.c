#include "movies_common.h"

#define MAX_CHILDREN 32 

typedef struct RNode {
    double min[K_DIMS], max[K_DIMS];
    struct RNode *children[MAX_CHILDREN];
    Movie *data[MAX_CHILDREN];
    int count;
    int is_leaf;
} RNode;

// RAM Calculation
long get_rtree_memory(RNode *n) {
    if (!n) return 0;
    long size = sizeof(RNode);
    if (!n->is_leaf) {
        for(int i=0; i<n->count; i++) size += get_rtree_memory(n->children[i]);
    }
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

int sort_dim = 0;
int cmp_dim(const void *a, const void *b) { 
    double v1 = (*(Movie**)a)->values[sort_dim];
    double v2 = (*(Movie**)b)->values[sort_dim];
    return (v1 > v2) - (v1 < v2);
}

void update_mbr(RNode *node) {
    for(int k=0; k<K_DIMS; k++) {
        node->min[k] = 1e15; node->max[k] = -1e15;
    }
    
    if (node->is_leaf) {
        for(int i=0; i<node->count; i++) {
            Movie *m = node->data[i];
            if (!m->is_deleted) { 
                for(int k=0; k<K_DIMS; k++) {
                    if(m->values[k] < node->min[k]) node->min[k] = m->values[k];
                    if(m->values[k] > node->max[k]) node->max[k] = m->values[k];
                }
            }
        }
    } else {
         for(int i=0; i<node->count; i++) {
             for(int k=0; k<K_DIMS; k++) {
                 if(node->children[i]->min[k] < node->min[k]) node->min[k] = node->children[i]->min[k];
                 if(node->children[i]->max[k] > node->max[k]) node->max[k] = node->children[i]->max[k];
             }
         }
    }
}

RNode* build_rtree(Movie **mptr, int n) {
    RNode *node = malloc(sizeof(RNode));
    node->count = 0; node->is_leaf = 1;
    
    if (n <= MAX_CHILDREN) {
        for(int i=0; i<n; i++) node->data[i] = mptr[i];
        node->count = n;
        update_mbr(node);
        return node;
    }
    
    node->is_leaf = 0;
    if (n > 1000) { 
        sort_dim = 0; 
        qsort(mptr, n, sizeof(Movie*), cmp_dim);
    }
    
    int step = n / MAX_CHILDREN; 
    if (n % MAX_CHILDREN != 0) step++; 
    if (step < 1) step = 1;
    
    int current = 0;
    while(current < n) {
        if (node->count >= MAX_CHILDREN) break;
        int remaining_slots = MAX_CHILDREN - node->count;
        int remaining_items = n - current;
        int chunk = remaining_items / remaining_slots;
        if (remaining_items % remaining_slots != 0) chunk++;
        
        int end = current + chunk;
        if (end > n) end = n;
        
        node->children[node->count++] = build_rtree(mptr + current, end - current);
        current = end;
    }
    update_mbr(node);
    return node;
}

void free_rtree(RNode *node) {
    if (!node) return;
    if (!node->is_leaf) {
        for(int i=0; i<node->count; i++) free_rtree(node->children[i]);
    }
    free(node);
}

void update_rtree(RNode *root, Movie *target, double new_pop) {
    printf(" [Update] Moved '%s' (Pop: %.2f -> %.2f)\n", target->title, target->values[1], new_pop);
    target->values[1] = new_pop;
}

void query_rtree(RNode *node, double min[], double max[], Movie **res, int *cnt) {
    if (!node) return;
    for(int k=0; k<K_DIMS; k++) {
        if (node->min[k] > max[k] || node->max[k] < min[k]) return;
    }
    if (node->is_leaf) {
        for(int i=0; i<node->count; i++) {
             Movie *m = node->data[i];
             if (!m->is_deleted) { 
                int match = 1;
                for(int k=0; k<K_DIMS; k++) {
                    if (m->values[k] < min[k] || m->values[k] > max[k]) { match=0; break; }
                }
                if (match) res[(*cnt)++] = m;
            }
        }
    } else {
        for(int i=0; i<node->count; i++) query_rtree(node->children[i], min, max, res, cnt);
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

    printf("\n=== R-Tree (%d Dimensions) ===\n", K_DIMS);
    printf("--------------------------------------------------------------------------\n");
    printf("| Size         | Build (s) | Insert (s) | Query (s) | Memory (MB) |\n");
    printf("--------------------------------------------------------------------------\n");

    int step = 20000;
    for(int n = step; n <= total_n; n += step) {
        for(int i=0; i<n; i++) ptrs[i] = &data[i];
        
        clock_t start = clock();
        RNode *root = build_rtree(ptrs, n);
        double build_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        double insert_time = 0.0; 

        int count = 0;
        start = clock();
        query_rtree(root, minv, maxv, results, &count);
        double query_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        long bytes = get_rtree_memory(root);
        double mem_mb = bytes / (1024.0 * 1024.0);
        
        printf("| %-12d | %-9.4f | %-10.4f | %-9.4f | %-11.2f |\n", n, build_time, insert_time, query_time, mem_mb);
        free_rtree(root);
    }
    printf("--------------------------------------------------------------------------\n");

    // DEMO
    for(int i=0; i<total_n; i++) ptrs[i] = &data[i];
    RNode *root = build_rtree(ptrs, total_n);
    int count = 0;
    query_rtree(root, minv, maxv, results, &count);
    printf("Query Found: %d movies\n", count);
    
    if (count > 0) {
        printf("\n[Delete Demo] Removing: '%s'\n", results[0]->title);
        results[0]->is_deleted = 1;
        
        printf("[Update Demo] Updating popularity...\n");
        if(count > 1) update_rtree(root, results[1], results[1]->values[1] + 10.0);
        
        int c2 = 0;
        query_rtree(root, minv, maxv, results, &c2); 
        
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
    
    free_rtree(root);
    free(data); free(ptrs); free(results);
    return 0;
}