#include "movies_common.h"

typedef struct KDNode {
    Movie *movie;
    struct KDNode *left, *right;
    int axis;
} KDNode;

// Comparators for qsort
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
    
    // Check match
    if (m->budget >= min[0] && m->budget <= max[0] &&
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
    int n = load_csv("movies.csv", data);
    if (n == 0) { generate_dummy_data(data, 10000); n = 10000; }

    Movie **ptrs = malloc(n * sizeof(Movie*));
    for(int i=0; i<n; i++) ptrs[i] = &data[i];

    printf("[k-d Tree] Building for %d movies...\n", n);
    clock_t start = clock();
    KDNode *root = build_kdtree(ptrs, n, 0);
    printf("Build Time: %.4f sec\n", (double)(clock()-start)/CLOCKS_PER_SEC);

    double minv[] = {1000, 2, 60}, maxv[] = {50000, 50, 180};
    Movie **results = malloc(n * sizeof(Movie*));
    int count = 0;
    
    printf("[k-d Tree] Querying...\n");
    start = clock();
    query_kdtree(root, minv, maxv, results, &count);
    printf("Query Time: %.4f sec | Found: %d\n", (double)(clock()-start)/CLOCKS_PER_SEC, count);
    
    if (count > 0) {
        printf("\n[LSH Similarity] Target: %s\n", results[0]->title);
        for(int i=1; i<count && i<10; i++) {
            double sim = jaccard_similarity(results[0], results[i]);
            if (sim > 0.3) printf(" -> %s (Sim: %.2f)\n", results[i]->title, sim);
        }
    }
    return 0;
}