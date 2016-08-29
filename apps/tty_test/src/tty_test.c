#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sos.h>
#include <sys/time.h>
#include <utils/time.h>

#define BUF_SIZ 4096 // would go higher, but terminal buffer size limit is 4096
#define SMALL_BUF_SZ 2

char test_str[] = "Basic test string for read/write";
char small_buf[SMALL_BUF_SZ];

void test_buffers(int console_fd) {
    int result;
    printf("test a small string from the code segment\n");
    result = sos_sys_write(console_fd, test_str, strlen(test_str));
    assert((unsigned int)result == strlen(test_str));

    printf("test reading to a small buffer\n");
    result = sos_sys_read(console_fd, small_buf, SMALL_BUF_SZ);
    /* make sure you type in at least SMALL_BUF_SZ */
    assert(result == SMALL_BUF_SZ);

    printf("test a reading into a large on-stack buffer\n");
    char stack_buf[BUF_SIZ];
    /* for this test you'll need to paste a lot of data into
       the console, without newlines */
    result = sos_sys_read(console_fd, stack_buf, BUF_SIZ);
    printf("%d == %d\n", result, BUF_SIZ);
    assert(result == BUF_SIZ);

    printf("Now attempt to write it\n");
    result = sos_sys_write(console_fd, stack_buf, BUF_SIZ);
    assert(result == BUF_SIZ);

    printf("this call to malloc should trigger an sbrk\n");
    char *heap_buf = malloc(BUF_SIZ);
    assert(heap_buf != NULL);

    /* for this test you'll need to paste a lot of data into
       the console, without newlines */
    result = sos_sys_read(console_fd, heap_buf, BUF_SIZ);
    assert(result == BUF_SIZ);

    result = sos_sys_write(console_fd, heap_buf, BUF_SIZ);
    assert(result == BUF_SIZ);

    printf("try sleeping\n");
    for (int i = 0; i < 5; i++) {
        time_t prev_seconds = time(NULL);
        sleep(1);
        time_t next_seconds = time(NULL);
        assert(next_seconds > prev_seconds);
        printf("Tick\n");
    }
}

int main(void){
    int in = open("console", O_RDWR);
    assert(in >= 0);
    test_buffers(in);

    while (1) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        printf("now: %6lds + %9ldns\n", now.tv_sec, now.tv_nsec);
        sleep(1);
    }

    return 0;
}
