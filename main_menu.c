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
        
        // ΑΛΛΑΓΗ: Καλούμε απευθείας τα .exe χωρίς gcc
        // (Βεβαιώσου ότι τα .exe ονομάζονται έτσι όπως φαίνονται παρακάτω)
        if (choice == 1) {
            printf("\n--- Running k-d Tree ---\n");
            system("tree_kdtree.exe"); 
        }
        else if (choice == 2) {
             printf("\n--- Running Quad Tree ---\n");
             system("tree_quad.exe");
        }
        else if (choice == 3) {
             printf("\n--- Running Range Tree ---\n");
             system("tree_range.exe");
        }
        else if (choice == 4) {
             printf("\n--- Running R-Tree ---\n");
             system("tree_rtree.exe");
        }
    }
    return 0;
}