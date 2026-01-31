#include "movies_common.h"

#define MAX_CHILDREN (1 << K_DIMS) 
#define MAX_DEPTH 30 

typedef struct QuadNode {
    double min[K_DIMS], max[K_DIMS]; 
    Movie *movies[50];     
    int count;
    struct QuadNode *children[MAX_CHILDREN]; 
    int is_leaf;
} QuadNode;

// RAM Calculation
long get_quad_memory(QuadNode *n) {
    if (!n) return 0;
    long size = sizeof(QuadNode);
    if (!n->is_leaf) {
        for(int i=0; i<MAX_CHILDREN; i++) size += get_quad_memory(n->children[i]);
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

QuadNode* create_node(double *min_c, double *max_c) {
    QuadNode *n = malloc(sizeof(QuadNode));
    for(int i=0; i<K_DIMS; i++) {
        n->min[i] = min_c[i];
        n->max[i] = max_c[i];
    }
    n->count = 0; n->is_leaf = 1;
    for(int i=0; i<MAX_CHILDREN; i++) n->children[i] = NULL;
    return n;
}

void free_quad(QuadNode *n) {
    if (!n) return;
    if (!n->is_leaf) {
        for(int i=0; i<MAX_CHILDREN; i++) free_quad(n->children[i]);
    }
    free(n);
}

int is_inside(Movie *m, double *min, double *max) {
    for(int i=0; i<K_DIMS; i++) {
        if (m->values[i] < min[i] || m->values[i] > max[i]) return 0;
    }
    return 1;
}

void insert_quad(QuadNode *n, Movie *m, int depth) {
    if (n->is_leaf) {
        if (n->count < 50 || depth > MAX_DEPTH) {
            if(n->count < 50) n->movies[n->count++] = m;
            return;
        }
        n->is_leaf = 0;
        double mid[K_DIMS];
        for(int d=0; d<K_DIMS; d++) mid[d] = (n->min[d] + n->max[d]) / 2.0;
        
        for(int i=0; i<MAX_CHILDREN; i++) {
            double c_min[K_DIMS], c_max[K_DIMS];
            for(int d=0; d<K_DIMS; d++) {
                if ((i >> d) & 1) { 
                    c_min[d] = mid[d]; c_max[d] = n->max[d]; 
                } else { 
                    c_min[d] = n->min[d]; c_max[d] = mid[d]; 
                }
            }
            n->children[i] = create_node(c_min, c_max);
        }
        
        for(int k=0; k<n->count; k++) {
            Movie *old_m = n->movies[k];
            int child_idx = 0;
            for(int d=0; d<K_DIMS; d++) {
                if (old_m->values[d] >= mid[d]) child_idx |= (1 << d);
            }
            insert_quad(n->children[child_idx], old_m, depth + 1);
        }
        n->count = 0;
    }
    
    if (!n->is_leaf) {
        double mid[K_DIMS];
        for(int d=0; d<K_DIMS; d++) mid[d] = (n->min[d] + n->max[d]) / 2.0;
        int child_idx = 0;
        for(int d=0; d<K_DIMS; d++) {
            if (m->values[d] >= mid[d]) child_idx |= (1 << d);
        }
        insert_quad(n->children[child_idx], m, depth + 1);
    }
}

void update_quad(QuadNode *root, Movie *target, double new_pop) {
    printf(" [Update] Moved '%s' (Pop: %.2f -> %.2f)\n", target->title, target->values[1], new_pop);
    target->values[1] = new_pop;
}

void query_quad(QuadNode *n, double min[], double max[], Movie **res, int *cnt) {
    if (!n) return;
    for(int i=0; i<K_DIMS; i++) {
        if (n->max[i] < min[i] || n->min[i] > max[i]) return;
    }

    if (n->is_leaf) {
        for(int i=0; i<n->count; i++) {
            Movie *m = n->movies[i];
            if (!m->is_deleted) {
                int match = 1;
                for(int d=0; d<K_DIMS; d++) {
                    if (m->values[d] < min[d] || m->values[d] > max[d]) { match = 0; break; }
                }
                if (match) res[(*cnt)++] = m;
            }
        }
    } else {
        for(int i=0; i<MAX_CHILDREN; i++) query_quad(n->children[i], min, max, res, cnt);
    }
}

int main() {
    Movie *data = malloc(MAX_MOVIES * sizeof(Movie));
    int total_n = load_csv("movies.csv", data);
    Movie **results = malloc(total_n * sizeof(Movie*));
    
    double root_min[K_DIMS], root_max[K_DIMS];
    for(int i=0; i<K_DIMS; i++) { 
        root_min[i] = -1000.0;     
        root_max[i] = 10000000000.0; 
    }
    
    // ΔΙΟΡΘΩΣΗ ΓΙΑ 2D
    double minv[K_DIMS], maxv[K_DIMS];
    minv[0] = 1000; maxv[0] = 50000; 
    if (K_DIMS > 1) { minv[1] = 2;    maxv[1] = 50; }
    if (K_DIMS > 2) { minv[2] = 60;   maxv[2] = 180; }
    for(int i=3; i<K_DIMS; i++) { minv[i] = -1e9; maxv[i] = 1e9; }

    printf("\n=== Generalized Quadtree (%d-D Trie) ===\n", K_DIMS);
    printf("--------------------------------------------------------------------------\n");
    printf("| Size         | Build (s) | Insert (s) | Query (s) | Memory (MB) |\n");
    printf("--------------------------------------------------------------------------\n");

    int step = 20000; 
    for(int n = step; n <= total_n; n += step) {
        QuadNode *root = create_node(root_min, root_max);
        
        clock_t start = clock();
        for(int i=0; i<n; i++) insert_quad(root, &data[i], 0);
        double build_time = (double)(clock()-start)/CLOCKS_PER_SEC;

        start = clock();
        insert_quad(root, &data[n-1], 0);
        double insert_time = (double)(clock()-start)/CLOCKS_PER_SEC;

        int count = 0;
        start = clock();
        query_quad(root, minv, maxv, results, &count);
        double query_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        long bytes = get_quad_memory(root);
        double mem_mb = bytes / (1024.0 * 1024.0);
        
        printf("| %-12d | %-9.4f | %-10.4f | %-9.4f | %-11.2f |\n", n, build_time, insert_time, query_time, mem_mb);
        free_quad(root);
    }
    printf("--------------------------------------------------------------------------\n");

    // DEMO
    QuadNode *root = create_node(root_min, root_max);
    for(int i=0; i<total_n; i++) insert_quad(root, &data[i], 0);
    
    int count = 0;
    query_quad(root, minv, maxv, results, &count);
    printf("Query Found: %d movies\n", count);

    if (count > 0) {
        printf("\n[Delete Demo] Removing: '%s'\n", results[0]->title);
        results[0]->is_deleted = 1;
        
        printf("[Update Demo] Updating popularity...\n");
        if (count > 1) update_quad(root, results[1], results[1]->values[1] + 10.0);

        int c2 = 0;
        query_quad(root, minv, maxv, results, &c2);
        
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
    free_quad(root);
    free(data); free(results);
    return 0;
}