#include "movies_common.h"

typedef struct RangeNode {
    Movie *movie;
    struct RangeNode *left, *right;
    Movie **sorted_y; // Δευτερεύουσα δομή
    int size;
} RangeNode;

int cmp_b(const void *a, const void *b) { return (*(Movie**)a)->budget > (*(Movie**)b)->budget ? 1 : -1; }
int cmp_p(const void *a, const void *b) { return (*(Movie**)a)->popularity > (*(Movie**)b)->popularity ? 1 : -1; }

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

void query_range(RangeNode *node, double min[], double max[], Movie **res, int *cnt) {
    if (!node) return;
    
    if (node->movie->budget >= min[0] && node->movie->budget <= max[0]) {
        // Απλοποιημένη αναζήτηση για το demo
        if (node->movie->popularity >= min[1] && node->movie->popularity <= max[1] &&
            node->movie->runtime >= min[2] && node->movie->runtime <= max[2]) {
             res[(*cnt)++] = node->movie;
        }
        query_range(node->left, min, max, res, cnt);
        query_range(node->right, min, max, res, cnt);
    } else if (node->movie->budget > max[0]) {
        query_range(node->left, min, max, res, cnt);
    } else {
        query_range(node->right, min, max, res, cnt);
    }
}

int main() {
    Movie *data = malloc(MAX_MOVIES * sizeof(Movie));
    int n = load_csv("movies.csv", data);
    
    Movie **ptrs = malloc(n * sizeof(Movie*));
    for(int i=0; i<n; i++) ptrs[i] = &data[i];

    printf("[Range Tree] Building...\n");
    clock_t start = clock();
    RangeNode *root = build_range(ptrs, n);
    printf("Build Time: %.4f sec\n", (double)(clock()-start)/CLOCKS_PER_SEC);

    double minv[] = {1000, 2, 60};     
    double maxv[] = {50000, 50, 180};  
    
    Movie **results = malloc(n * sizeof(Movie*));
    int count = 0;
    
    start = clock();
    query_range(root, minv, maxv, results, &count);
    printf("Query Time: %.4f sec | Found: %d\n", (double)(clock()-start)/CLOCKS_PER_SEC, count);
    
    // --- LSH SIMILARITY PART ---
    if (count > 0) {
        printf("\n[LSH Similarity] Target: %s\n", results[0]->title);
        for(int i=1; i<count && i<10; i++) {
            double sim = jaccard_similarity(results[0], results[i]);
            if (sim > 0.3) printf(" -> %s (Sim: %.2f)\n", results[i]->title, sim);
        }
    }
    return 0;
}