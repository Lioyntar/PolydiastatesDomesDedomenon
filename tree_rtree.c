#include "movies_common.h"

// --- R-TREE ---
typedef struct RNode {
    double min[3], max[3];
    struct RNode *children[100];
    Movie *data[100];
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
            if (!m->is_deleted) { // MBR considers only non-deleted ideally, but static build includes all
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

RNode* build_rtree(Movie **mptr, int n) {
    RNode *node = malloc(sizeof(RNode));
    node->count = 0; node->is_leaf = 1;
    if (n <= 100) {
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
        if (node->count < 100) node->children[node->count++] = build_rtree(mptr + i, end - i);
    }
    update_mbr(node);
    return node;
}

void query_rtree(RNode *node, double min[], double max[], Movie **res, int *cnt) {
    if (!node) return;
    if (node->min[0] > max[0] || node->max[0] < min[0] ||
        node->min[1] > max[1] || node->max[1] < min[1] ||
        node->min[2] > max[2] || node->max[2] < min[2]) return;

    if (node->is_leaf) {
        for(int i=0; i<node->count; i++) {
             Movie *m = node->data[i];
             if (!m->is_deleted && // Check Delete
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

int main() {
    Movie *data = malloc(MAX_MOVIES * sizeof(Movie));
    int total_n = load_csv("movies.csv", data);
    Movie **ptrs = malloc(total_n * sizeof(Movie*));
    Movie **results = malloc(total_n * sizeof(Movie*));
    
    double minv[] = {1000, 2, 60};     
    double maxv[] = {50000, 50, 180};  

    printf("\n=== EXPERIMENTAL EVALUATION (Scalability) ===\n");
    printf("| Dataset Size | Build Time (s) | Query Time (s) |\n");
    printf("|--------------|----------------|----------------|\n");

    for (int n = 50000; n <= 200000; n += 50000) {
        if (n > total_n) break;
        for(int i=0; i<n; i++) ptrs[i] = &data[i];
        
        clock_t start = clock();
        RNode *root = build_rtree(ptrs, n);
        double build_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        int count = 0;
        start = clock();
        query_rtree(root, minv, maxv, results, &count);
        double query_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        printf("| %-12d | %-14.4f | %-14.4f |\n", n, build_time, query_time);
    }

    // Demo Operations
    for(int i=0; i<total_n; i++) ptrs[i] = &data[i];
    RNode *root = build_rtree(ptrs, total_n);
    int count = 0;
    query_rtree(root, minv, maxv, results, &count);
    
    if (count > 0) {
        printf("\n[Delete Demo] Removing first result...\n");
        results[0]->is_deleted = 1;
        int c2 = 0;
        query_rtree(root, minv, maxv, results, &c2);
        printf("Count before: %d, After: %d\n", count, c2);
        
        results[0]->is_deleted = 0;
        run_knn(results[0], results, count, 5);
    }
    return 0;
}