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
        
        if (scanf("%d", &choice) != 1) { // Πιο ασφαλής ανάγνωση
            printf("Invalid input. Please enter a number.\n");
            // Καθαρισμός buffer για αποφυγή ατέρμονου βρόχου
            while(getchar() != '\n');
            continue;
        }

        if (choice == 0) break;
        
        // Καλούμε απευθείας τα .exe (υποθέτουμε ότι είναι στον ίδιο φάκελο)
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
        } else {
             printf("Invalid choice. Please select from 0 to 4.\n");
        }
    }
    return 0;
}