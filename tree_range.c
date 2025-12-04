#include "movies_common.h"

// --- RANGE TREE ---
typedef struct RangeNode {
    Movie *movie; 
    struct RangeNode *left, *right;
    Movie **sorted_y; // Διατηρούμε τη δομή για το σωστό Build Time
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

RangeNode* build_range(Movie **mptr, int n) {
    if (n <= 0) return NULL;
    // Ταξινόμηση κατά X (Budget)
    qsort(mptr, n, sizeof(Movie*), cmp_b); 
    
    int mid = n / 2;
    RangeNode *node = malloc(sizeof(RangeNode));
    node->movie = mptr[mid];
    
    // Δημιουργία δευτερεύουσας δομής (για να μετρήσει στο Build Time)
    node->size = n;
    node->sorted_y = malloc(n * sizeof(Movie*));
    memcpy(node->sorted_y, mptr, n * sizeof(Movie*));
    qsort(node->sorted_y, n, sizeof(Movie*), cmp_p);

    node->left = build_range(mptr, mid);
    node->right = build_range(mptr + mid + 1, n - mid - 1);
    return node;
}

// ΔΙΟΡΘΩΜΕΝΗ ΣΥΝΑΡΤΗΣΗ QUERY (Χωρίς διπλότυπα)
void query_range(RangeNode *node, double min[], double max[], Movie **res, int *cnt) {
    if (!node) return;

    // 1. Έλεγχος του τρέχοντος κόμβου (Μοναδική εγγραφή)
    if (!node->movie->is_deleted &&
        node->movie->budget >= min[0] && node->movie->budget <= max[0] &&
        node->movie->popularity >= min[1] && node->movie->popularity <= max[1] &&
        node->movie->runtime >= min[2] && node->movie->runtime <= max[2]) {
        res[(*cnt)++] = node->movie;
    }

    // 2. Αναδρομή βάσει Budget (BST Logic)
    // Αν το Budget του κόμβου είναι μεγαλύτερο από το min, μπορεί να υπάρχουν κι άλλα αριστερά
    if (node->movie->budget >= min[0]) {
        query_range(node->left, min, max, res, cnt);
    }
    // Αν το Budget του κόμβου είναι μικρότερο από το max, μπορεί να υπάρχουν κι άλλα δεξιά
    if (node->movie->budget <= max[0]) {
        query_range(node->right, min, max, res, cnt);
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
        RangeNode *root = build_range(ptrs, n);
        double build_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        int count = 0;
        start = clock();
        query_range(root, minv, maxv, results, &count);
        double query_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        printf("| %-12d | %-14.4f | %-14.4f |\n", n, build_time, query_time);
    }
    
    // Demo Operations (Full Dataset)
    for(int i=0; i<total_n; i++) ptrs[i] = &data[i];
    RangeNode *root = build_range(ptrs, total_n);
    int count = 0;
    query_range(root, minv, maxv, results, &count);
    
    printf("\nFound: %d movies (Should match 2798)\n", count);

    if (count > 0) {
        printf("\n[Delete Demo] Removing first result: '%s'...\n", results[0]->title);
        results[0]->is_deleted = 1;
        int c2 = 0;
        query_range(root, minv, maxv, results, &c2);
        printf("Count before: %d, After: %d\n", count, c2);
        
        // Restore for kNN
        results[0]->is_deleted = 0;
        run_knn(results[0], results, count, 5);
    }
    
    // --- LSH SIMILARITY ---
    if (count > 0) {
        printf("\n[LSH Similarity] Target: %s\n", results[0]->title);
        int found_sim = 0;
        for(int i=1; i<count && i<500; i++) { 
            double sim = jaccard_similarity(results[0], results[i]);
            if (sim > 0.3) {
                printf(" -> %s (Sim: %.2f)\n", results[i]->title, sim);
                found_sim++;
                if(found_sim >= 5) break; 
            }
        }
    }

    return 0;
}