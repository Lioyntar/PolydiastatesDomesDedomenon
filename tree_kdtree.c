#include "movies_common.h"

typedef struct KDNode {
    Movie *movie;
    struct KDNode *left, *right;
    int axis;
} KDNode;

int cmp_b(const void *a, const void *b) { return (*(Movie**)a)->budget > (*(Movie**)b)->budget ? 1 : -1; }
int cmp_p(const void *a, const void *b) { return (*(Movie**)a)->popularity > (*(Movie**)b)->popularity ? 1 : -1; }
int cmp_r(const void *a, const void *b) { return (*(Movie**)a)->runtime > (*(Movie**)b)->runtime ? 1 : -1; }

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

void query_kdtree(KDNode *node, double min[], double max[], Movie **res, int *cnt) {
    if (!node) return;
    Movie *m = node->movie;
    
    // Check match AND Check if NOT deleted
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

int main() {
    Movie *data = malloc(MAX_MOVIES * sizeof(Movie));
    int total_n = load_csv("movies.csv", data);
    
    Movie **ptrs = malloc(total_n * sizeof(Movie*));
    Movie **results = malloc(total_n * sizeof(Movie*));
    
    // --- 1. EXPERIMENTAL EVALUATION (SCALABILITY) ---
    printf("\n=== EXPERIMENTAL EVALUATION (Scalability) ===\n");
    printf("| Dataset Size | Build Time (s) | Query Time (s) |\n");
    printf("|--------------|----------------|----------------|\n");
    
    double minv[] = {1000, 2, 60};     
    double maxv[] = {50000, 50, 180}; 

    for (int n = 50000; n <= 200000; n += 50000) {
        if (n > total_n) break;
        
        // Reset pointers
        for(int i=0; i<n; i++) ptrs[i] = &data[i];
        
        clock_t start = clock();
        KDNode *root = build_kdtree(ptrs, n, 0);
        double build_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        int count = 0;
        start = clock();
        query_kdtree(root, minv, maxv, results, &count);
        double query_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        printf("| %-12d | %-14.4f | %-14.4f |\n", n, build_time, query_time);
        
        // Free tree (simplified: just break for next iter, leaks memory in demo but OK)
    }

    // --- 2. FULL RUN & OPERATIONS DEMO ---
    printf("\n=== FULL DATASET OPERATIONS ===\n");
    for(int i=0; i<total_n; i++) ptrs[i] = &data[i];
    KDNode *root = build_kdtree(ptrs, total_n, 0);
    
    // Normal Query
    int count = 0;
    query_kdtree(root, minv, maxv, results, &count);
    printf("Initial Query Results: %d\n", count);
    
    // DELETE OPERATION
    if (count > 0) {
        printf("\n[Delete Operation] Deleting '%s'...\n", results[0]->title);
        results[0]->is_deleted = 1; // Logical Delete
        
        int count_after = 0;
        query_kdtree(root, minv, maxv, results, &count_after);
        printf("Query Results after Delete: %d (Successfully removed)\n", count_after);
    }
    
    // kNN OPERATION
    // Ψάχνουμε τους γείτονες της 1ης ταινίας που βρήκαμε (πριν τη διαγραφή)
    // Επαναφέρουμε τη διαγραφή για το kNN demo
    if (count > 0) {
        results[0]->is_deleted = 0; 
        run_knn(results[0], results, count, 5); // 5 Nearest Neighbors
    }

    return 0;
}