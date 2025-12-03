#include "movies_common.h"

typedef struct QuadNode {
    double x_min, y_min, x_max, y_max;
    Movie *movies[50]; // Capacity
    int count;
    struct QuadNode *nw, *ne, *sw, *se;
    int is_leaf;
} QuadNode;

QuadNode* create_node(double x1, double y1, double x2, double y2) {
    QuadNode *n = malloc(sizeof(QuadNode));
    n->x_min = x1; n->y_min = y1; n->x_max = x2; n->y_max = y2;
    n->count = 0; n->is_leaf = 1;
    n->nw = n->ne = n->sw = n->se = NULL;
    return n;
}

void insert_quad(QuadNode *n, Movie *m) {
    if (m->budget < n->x_min || m->budget > n->x_max || 
        m->popularity < n->y_min || m->popularity > n->y_max) return;

    if (n->is_leaf) {
        if (n->count < 50) { n->movies[n->count++] = m; return; }
        // Split
        n->is_leaf = 0;
        double xm = (n->x_min + n->x_max)/2, ym = (n->y_min + n->y_max)/2;
        n->nw = create_node(n->x_min, ym, xm, n->y_max);
        n->ne = create_node(xm, ym, n->x_max, n->y_max);
        n->sw = create_node(n->x_min, n->y_min, xm, ym);
        n->se = create_node(xm, n->y_min, n->x_max, ym);
        // Re-distribute existing
        for(int i=0; i<n->count; i++) {
            insert_quad(n->nw, n->movies[i]); insert_quad(n->ne, n->movies[i]);
            insert_quad(n->sw, n->movies[i]); insert_quad(n->se, n->movies[i]);
        }
        n->count = 0;
    }
    insert_quad(n->nw, m); insert_quad(n->ne, m);
    insert_quad(n->sw, m); insert_quad(n->se, m);
}

void query_quad(QuadNode *n, double min[], double max[], Movie **res, int *cnt) {
    if (!n) return;
    // Check overlap
    if (n->x_max < min[0] || n->x_min > max[0] || n->y_max < min[1] || n->y_min > max[1]) return;

    if (n->is_leaf) {
        for(int i=0; i<n->count; i++) {
            Movie *m = n->movies[i];
            if (m->budget >= min[0] && m->budget <= max[0] &&
                m->popularity >= min[1] && m->popularity <= max[1] &&
                m->runtime >= min[2] && m->runtime <= max[2]) {
                res[(*cnt)++] = m;
            }
        }
    } else {
        query_quad(n->nw, min, max, res, cnt); query_quad(n->ne, min, max, res, cnt);
        query_quad(n->sw, min, max, res, cnt); query_quad(n->se, min, max, res, cnt);
    }
}

int main() {
    Movie *data = malloc(MAX_MOVIES * sizeof(Movie));
    int n = load_csv("movies.csv", data);
    
    printf("[Quad Tree] Building...\n");
    clock_t start = clock();
    QuadNode *root = create_node(0, 0, 500000000, 1000); // Όρια για το QuadTree
    for(int i=0; i<n; i++) insert_quad(root, &data[i]);
    printf("Build Time: %.4f sec\n", (double)(clock()-start)/CLOCKS_PER_SEC);

    // ΟΡΙΑ ΑΝΑΖΗΤΗΣΗΣ (Πρέπει να είναι ίδια με το k-d tree για να βγάλει 2798)
    // Αν στο k-d tree άλλαξες τα όρια, άλλαξέ τα κι εδώ!
    // Εδώ βάζω τα όρια που φαίνεται να χρησιμοποίησες (βάσει του ότι βρήκε αποτελέσματα)
    double minv[] = {1000, 2, 60};     
    double maxv[] = {50000, 50, 180};  
    
    Movie **results = malloc(n * sizeof(Movie*));
    int count = 0;
    
    start = clock();
    query_quad(root, minv, maxv, results, &count);
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