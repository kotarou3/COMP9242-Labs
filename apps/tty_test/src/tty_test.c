#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#define PAGE_SIZE 4096

int main(void){
    printf("Performing massive malloc\n");
    char* x = malloc(1U<<30); // 2GB
    assert(x != NULL);
    printf("Performed massive malloc\n");
    for (unsigned int i = 0; i < 1U<<30; i += PAGE_SIZE) {
        x[i] = i;
        if (i % 1U<<25)
            printf("Allocated %u MB\n", i / (1U<<20));
    }
    printf("Task complete\n");
    return 0;
}
