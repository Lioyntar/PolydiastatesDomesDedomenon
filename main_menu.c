#include <stdio.h>
#include <stdlib.h>

int main() {
    int choice;
    while(1) {
        printf("\n==================================\n");
        printf("   C IMPLEMENTATION: MOVIE TREES  \n");
        printf("==================================\n");
        printf("1. Run k-d Tree\n");
        printf("2. Run Quad Tree\n");
        printf("3. Run Range Tree\n");
        printf("4. Run R-Tree\n");
        printf("0. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        if (choice == 0) break;
        
        // Χρήση system() για compile & run on the fly
        // Προϋποθέτει ότι έχεις GCC εγκατεστημένο
        if (choice == 1) {
            printf("\n--- Running k-d Tree ---\n");
            system("gcc tree_kdtree.c -o kdtree && kdtree"); 
            // Αν είσαι σε Linux βάλε ./kdtree, σε Windows kdtree
        }
        else if (choice == 2) {
             printf("\n--- Running Quad Tree ---\n");
             system("gcc tree_quad.c -o quadtree && quadtree");
        }
        else if (choice == 3) {
             printf("\n--- Running Range Tree ---\n");
             system("gcc tree_range.c -o rangetree && rangetree");
        }
        else if (choice == 4) {
             printf("\n--- Running R-Tree ---\n");
             system("gcc tree_rtree.c -o rtree && rtree");
        }
    }
    return 0;
}