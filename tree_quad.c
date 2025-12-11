#include "movies_common.h"

// --- OCTREE (3D) ---
typedef struct OctNode {
    double min[3], max[3]; 
    Movie *movies[50];     
    int count;
    struct OctNode *children[8]; 
    int is_leaf;
} OctNode;

OctNode* create_node(double x1, double y1, double z1, double x2, double y2, double z2) {
    OctNode *n = malloc(sizeof(OctNode));
    n->min[0] = x1; n->min[1] = y1; n->min[2] = z1;
    n->max[0] = x2; n->max[1] = y2; n->max[2] = z2;
    n->count = 0; n->is_leaf = 1;
    for(int i=0; i<8; i++) n->children[i] = NULL;
    return n;
}

// --- MEMORY DEALLOCATION (NEW) ---
void free_oct(OctNode *n) {
    if (!n) return;
    if (!n->is_leaf) {
        for(int i=0; i<8; i++) free_oct(n->children[i]);
    }
    free(n);
}

void insert_oct(OctNode *n, Movie *m) {
    if (m->budget < n->min[0] || m->budget > n->max[0] || 
        m->popularity < n->min[1] || m->popularity > n->max[1] ||
        m->runtime < n->min[2] || m->runtime > n->max[2]) return;

    if (n->is_leaf) {
        if (n->count < 50) { n->movies[n->count++] = m; return; }
        n->is_leaf = 0;
        double mid[3];
        for(int d=0; d<3; d++) mid[d] = (n->min[d] + n->max[d]) / 2.0;
        
        for(int i=0; i<8; i++) {
            double c_min[3], c_max[3];
            for(int d=0; d<3; d++) {
                if (i & (1 << d)) { c_min[d] = mid[d]; c_max[d] = n->max[d]; }
                else { c_min[d] = n->min[d]; c_max[d] = mid[d]; }
            }
            n->children[i] = create_node(c_min[0], c_min[1], c_min[2], c_max[0], c_max[1], c_max[2]);
        }
        for(int k=0; k<n->count; k++) {
            for(int i=0; i<8; i++) insert_oct(n->children[i], n->movies[k]);
        }
        n->count = 0;
    }
    for(int i=0; i<8; i++) insert_oct(n->children[i], m);
}

// --- STRUCTURAL UPDATE ---
void update_oct(OctNode *root, Movie *target, double new_pop) {
    target->is_deleted = 1; 
    Movie *new_m = malloc(sizeof(Movie));
    *new_m = *target;
    new_m->popularity = new_pop;
    new_m->is_deleted = 0;
    insert_oct(root, new_m);
    printf(" [Structural Update] Re-inserted '%s' with Pop: %.2f\n", target->title, new_pop);
}

void query_oct(OctNode *n, double min[], double max[], Movie **res, int *cnt) {
    if (!n) return;
    if (n->max[0] < min[0] || n->min[0] > max[0] || 
        n->max[1] < min[1] || n->min[1] > max[1] ||
        n->max[2] < min[2] || n->min[2] > max[2]) return;

    if (n->is_leaf) {
        for(int i=0; i<n->count; i++) {
            Movie *m = n->movies[i];
            if (!m->is_deleted &&
                m->budget >= min[0] && m->budget <= max[0] &&
                m->popularity >= min[1] && m->popularity <= max[1] &&
                m->runtime >= min[2] && m->runtime <= max[2]) {
                res[(*cnt)++] = m;
            }
        }
    } else {
        for(int i=0; i<8; i++) query_oct(n->children[i], min, max, res, cnt);
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
    Movie **results = malloc(total_n * sizeof(Movie*));
    
    double minv[] = {1000, 2, 60};     
    double maxv[] = {50000, 50, 180}; 

    printf("\n=== EXPERIMENTAL EVALUATION (Scalability) ===\n");
    printf("| Dataset Size | Build (s) | Insert (ms)| Query (s) |\n");
    printf("|--------------|-----------|------------|-----------|\n");

    for (int n = 50000; n <= 200000; n += 50000) {
        if (n > total_n) break;
        
        clock_t start = clock();
        OctNode *root = create_node(0, 0, 0, 500000000, 1000, 500);
        for(int i=0; i<n; i++) insert_oct(root, &data[i]);
        double build_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        start = clock();
        for(int k=0; k<100; k++) insert_oct(root, &data[rand()%n]);
        double insert_time = ((double)(clock()-start)/CLOCKS_PER_SEC) * 1000.0 / 100.0;

        int count = 0;
        start = clock();
        query_oct(root, minv, maxv, results, &count);
        double query_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        
        printf("| %-12d | %-9.4f | %-10.4f | %-9.4f |\n", n, build_time, insert_time, query_time);
        free_oct(root);
    }

    // Demo
    OctNode *root = create_node(0, 0, 0, 500000000, 1000, 500);
    for(int i=0; i<total_n; i++) insert_oct(root, &data[i]);
    
    int count = 0;
    query_oct(root, minv, maxv, results, &count);
    
    if (count > 0) {
        printf("\n[Delete Demo] Removing: '%s'\n", results[0]->title);
        printf("\n[Update Demo] Updating popularity...\n");
        update_oct(root, results[0], results[0]->popularity + 20.0);
        
        int c2 = 0;
        query_oct(root, minv, maxv, results, &c2); 
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
    free_oct(root);
    free(data);
    free(results);
    return 0;
}