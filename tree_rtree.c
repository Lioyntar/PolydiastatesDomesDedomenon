#include "movies_common.h"

#define MAX_CHILDREN 100

typedef struct RNode {
    double min[3], max[3];
    struct RNode *children[MAX_CHILDREN];
    Movie *data[MAX_CHILDREN];
    int count;
    int is_leaf;
} RNode;

int cmp_b(const void *a, const void *b) { return (*(Movie**)a)->budget > (*(Movie**)b)->budget ? 1 : -1; }

void update_mbr(RNode *node) {
    node->min[0] = 1e9; node->min[1] = 1e9; node->min[2] = 1e9;
    node->max[0] = -1;  node->max[1] = -1;  node->max[2] = -1;
    
    if (node->is_leaf) {
        for(int i=0; i<node->count; i++) {
            Movie *m = node->data[i];
            if (!m->is_deleted) { 
                if(m->budget < node->min[0]) node->min[0] = m->budget;
                if(m->budget > node->max[0]) node->max[0] = m->budget;
                if(m->popularity < node->min[1]) node->min[1] = m->popularity;
                if(m->popularity > node->max[1]) node->max[1] = m->popularity;
                if(m->runtime < node->min[2]) node->min[2] = m->runtime;
                if(m->runtime > node->max[2]) node->max[2] = m->runtime;
            }
        }
    } else {
         for(int i=0; i<node->count; i++) {
             for(int d=0; d<3; d++) {
                 if(node->children[i]->min[d] < node->min[d]) node->min[d] = node->children[i]->min[d];
                 if(node->children[i]->max[d] > node->max[d]) node->max[d] = node->children[i]->max[d];
             }
         }
    }
}

// Bulk Build
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
    qsort(mptr, n, sizeof(Movie*), cmp_b);
    int step = n / 10; if (step == 0) step = 1; 
    for(int i=0; i<n; i+=step) {
        int end = (i + step > n) ? n : i + step;
        if (node->count < MAX_CHILDREN) node->children[node->count++] = build_rtree(mptr + i, end - i);
    }
    update_mbr(node);
    return node;
}

// --- MEMORY DEALLOCATION (NEW) ---
void free_rtree(RNode *node) {
    if (!node) return;
    if (!node->is_leaf) {
        for(int i=0; i<node->count; i++) free_rtree(node->children[i]);
    }
    free(node);
}

// --- DYNAMIC INSERT ---
int insert_rtree(RNode *node, Movie *m) {
    if (node->is_leaf) {
        if (node->count < MAX_CHILDREN) {
            node->data[node->count++] = m;
            update_mbr(node);
            return 1;
        } else {
            return 0; // Simple overflow handling
        }
    } else {
        for(int i=0; i<node->count; i++) {
            if (insert_rtree(node->children[i], m)) {
                update_mbr(node);
                return 1;
            }
        }
    }
    return 0;
}

void insert_rtree_wrapper(RNode *root, Movie *m) {
    insert_rtree(root, m);
}

// --- STRUCTURAL UPDATE ---
void update_rtree(RNode *root, Movie *target, double new_pop) {
    target->is_deleted = 1;
    Movie *new_m = malloc(sizeof(Movie));
    *new_m = *target;
    new_m->popularity = new_pop;
    new_m->is_deleted = 0;
    insert_rtree_wrapper(root, new_m);
    printf(" [Structural Update] R-Tree updated for '%s'\n", target->title);
}

void query_rtree(RNode *node, double min[], double max[], Movie **res, int *cnt) {
    if (!node) return;
    if (node->min[0] > max[0] || node->max[0] < min[0] ||
        node->min[1] > max[1] || node->max[1] < min[1] ||
        node->min[2] > max[2] || node->max[2] < min[2]) return;

    if (node->is_leaf) {
        for(int i=0; i<node->count; i++) {
             Movie *m = node->data[i];
             if (!m->is_deleted && 
                m->budget >= min[0] && m->budget <= max[0] &&
                m->popularity >= min[1] && m->popularity <= max[1] &&
                m->runtime >= min[2] && m->runtime <= max[2]) {
                res[(*cnt)++] = m;
            }
        }
    } else {
        for(int i=0; i<node->count; i++) query_rtree(node->children[i], min, max, res, cnt);
    }
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
        RNode *root = build_rtree(ptrs, n);
        double build_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        start = clock();
        for(int k=0; k<100; k++) insert_rtree_wrapper(root, &data[rand()%n]);
        double insert_time = ((double)(clock()-start)/CLOCKS_PER_SEC) * 1000.0 / 100.0;
        
        int count = 0;
        start = clock();
        query_rtree(root, minv, maxv, results, &count);
        double query_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        printf("| %-12d | %-9.4f | %-10.4f | %-9.4f |\n", n, build_time, insert_time, query_time);
        free_rtree(root);
    }

    // Demo
    for(int i=0; i<total_n; i++) ptrs[i] = &data[i];
    RNode *root = build_rtree(ptrs, total_n);
    int count = 0;
    query_rtree(root, minv, maxv, results, &count);
    
    if (count > 0) {
        printf("\n[Delete Demo] Removing: '%s'\n", results[0]->title);
        printf("\n[Update Demo] Updating popularity...\n");
        update_rtree(root, results[0], results[0]->popularity + 10.0);
        
        int c2 = 0;
        query_rtree(root, minv, maxv, results, &c2); 
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
    free_rtree(root);
    free(data);
    free(ptrs);
    free(results);
    return 0;
}