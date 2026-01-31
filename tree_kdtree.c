#include "movies_common.h"

typedef struct KDNode {
    Movie *movie;
    struct KDNode *left, *right;
    int axis;
} KDNode;

int current_axis_sort = 0;

// Function to calculate memory usage
long count_nodes(KDNode *node) {
    if (!node) return 0;
    return 1 + count_nodes(node->left) + count_nodes(node->right);
}

int cmp_dynamic(const void *a, const void *b) {
    Movie *m1 = *(Movie**)a;
    Movie *m2 = *(Movie**)b;
    if (m1->values[current_axis_sort] > m2->values[current_axis_sort]) return 1;
    if (m1->values[current_axis_sort] < m2->values[current_axis_sort]) return -1;
    return 0;
}

KDNode* build_kdtree(Movie **mptr, int n, int depth) {
    if (n <= 0) return NULL;
    int axis = depth % K_DIMS;
    current_axis_sort = axis;
    qsort(mptr, n, sizeof(Movie*), cmp_dynamic);

    int mid = n / 2;
    KDNode *node = malloc(sizeof(KDNode));
    node->movie = mptr[mid];
    node->axis = axis;
    node->left = build_kdtree(mptr, mid, depth + 1);
    node->right = build_kdtree(mptr + mid + 1, n - mid - 1, depth + 1);
    return node;
}

KDNode* insert_kdtree(KDNode *node, Movie *m, int depth) {
    if (!node) {
        KDNode *n = malloc(sizeof(KDNode));
        n->movie = m;
        n->axis = depth % K_DIMS;
        n->left = n->right = NULL;
        return n;
    }
    int axis = node->axis;
    if (m->values[axis] < node->movie->values[axis]) 
        node->left = insert_kdtree(node->left, m, depth + 1);
    else 
        node->right = insert_kdtree(node->right, m, depth + 1);
    return node;
}

void free_kdtree(KDNode *node) {
    if (!node) return;
    free_kdtree(node->left);
    free_kdtree(node->right);
    free(node);
}

void update_kdtree(KDNode **root, Movie *target, double new_pop) {
    target->is_deleted = 1; 
    Movie *new_m = malloc(sizeof(Movie));
    *new_m = *target; 
    new_m->values[1] = new_pop; 
    new_m->is_deleted = 0;
    *root = insert_kdtree(*root, new_m, 0);
    printf(" [Update] Moved '%s' (Pop: %.2f -> %.2f)\n", target->title, target->values[1], new_pop);
}

void query_kdtree(KDNode *node, double min[], double max[], Movie **res, int *cnt) {
    if (!node) return;
    Movie *m = node->movie;
    if (!m->is_deleted) {
        int match = 1;
        for(int i=0; i<K_DIMS; i++) {
            if (m->values[i] < min[i] || m->values[i] > max[i]) {
                match = 0; break;
            }
        }
        if (match) res[(*cnt)++] = m;
    }
    double val = m->values[node->axis];
    if (val >= min[node->axis]) query_kdtree(node->left, min, max, res, cnt);
    if (val <= max[node->axis]) query_kdtree(node->right, min, max, res, cnt);
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

int main() {
    Movie *data = malloc(MAX_MOVIES * sizeof(Movie));
    int total_n = load_csv("movies.csv", data);
    
    Movie **ptrs = malloc(total_n * sizeof(Movie*));
    Movie **results = malloc(total_n * sizeof(Movie*));
    
    // ΔΙΟΡΘΩΣΗ: Ασφαλής ορισμός ορίων για 2 διαστάσεις
    double minv[K_DIMS], maxv[K_DIMS];
    minv[0] = 1000; maxv[0] = 50000; 
    
    if (K_DIMS > 1) { minv[1] = 2;    maxv[1] = 50; }
    if (K_DIMS > 2) { minv[2] = 60;   maxv[2] = 180; }
    
    for(int i=3; i<K_DIMS; i++) { minv[i] = -1e9; maxv[i] = 1e9; }

    printf("\n=== k-d Tree (%d Dimensions) ===\n", K_DIMS);
    printf("--------------------------------------------------------------------------\n");
    printf("| Size         | Build (s) | Insert (s) | Query (s) | Memory (MB) |\n");
    printf("--------------------------------------------------------------------------\n");

    int step = 20000; 
    for(int n = step; n <= total_n; n += step) {
        for(int i=0; i<n; i++) ptrs[i] = &data[i];

        clock_t start = clock();
        KDNode *root = build_kdtree(ptrs, n, 0);
        double build_time = (double)(clock()-start)/CLOCKS_PER_SEC;

        start = clock();
        insert_kdtree(root, &data[n-1], 0);
        double insert_time = (double)(clock()-start)/CLOCKS_PER_SEC;

        int count = 0;
        start = clock();
        query_kdtree(root, minv, maxv, results, &count);
        double query_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        long nodes = count_nodes(root);
        double mem_mb = (nodes * sizeof(KDNode)) / (1024.0 * 1024.0);
        
        printf("| %-12d | %-9.4f | %-10.4f | %-9.4f | %-11.2f |\n", n, build_time, insert_time, query_time, mem_mb);
        free_kdtree(root);
    }
    printf("--------------------------------------------------------------------------\n");
    fflush(stdout);

    // FULL DEMO
    for(int i=0; i<total_n; i++) ptrs[i] = &data[i];
    KDNode *root = build_kdtree(ptrs, total_n, 0);

    int count = 0;
    query_kdtree(root, minv, maxv, results, &count);
    printf("\nQuery Found: %d movies\n", count);
    
    if (count > 0) {
        printf("\n[Delete Demo] Removing: '%s'\n", results[0]->title);
        results[0]->is_deleted = 1;
        fflush(stdout);
        
        printf("[Update Demo] Updating popularity...\n");
        fflush(stdout);
        
        if(count > 1) update_kdtree(&root, results[1], results[1]->values[1] + 15.0);
        
        int c2 = 0;
        query_kdtree(root, minv, maxv, results, &c2);
        
        if (c2 > 0) run_knn(results[0], results, c2, 5);

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
    free_kdtree(root);
    free(data); free(ptrs); free(results);
    return 0;
}