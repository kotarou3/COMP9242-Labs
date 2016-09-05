#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main(void){
    while (1) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        printf("now: %6lds + %9ldns\n", now.tv_sec, now.tv_nsec);
        sleep(1);
    }

    return 0;
}
