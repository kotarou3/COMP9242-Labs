#include <__config>
#undef _LIBCPP_HAS_NO_THREADS
#include <atomic>
#define _LIBCPP_HAS_NO_THREADS
#include <random>
#include <cstdio>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>

extern "C" {
    #include <sos.h>
}

std::mt19937 rng;

constexpr uint8_t* AREA1_START = reinterpret_cast<uint8_t*>(0x1000000);
constexpr uint8_t* AREA1_END =   reinterpret_cast<uint8_t*>(0x1002000);
constexpr uint8_t* AREA2_START = reinterpret_cast<uint8_t*>(0x2000000);
constexpr uint8_t* AREA2_END =   reinterpret_cast<uint8_t*>(0x2002000);

int main() {
    printf("Calling sos_share_vm()\n");
    assert(sos_share_vm(AREA1_START, AREA1_END - AREA1_START, true) == 0);
    assert(sos_share_vm(AREA2_START, AREA2_END - AREA2_START, false) == 0);

    auto& parentFlag = reinterpret_cast<std::atomic_flag*>(AREA1_START)[0];
    auto& bufferLock = reinterpret_cast<std::atomic_flag*>(AREA1_START)[1];
    bool& bufferRead = *reinterpret_cast<bool*>(AREA1_START + 0x1000);
    size_t* buffer = reinterpret_cast<size_t*>(AREA2_START);
    if (!parentFlag.test_and_set()) {
        bufferRead = true;

        printf("In parent. Spawning child\n");
        assert(sos_process_create("share_vm_test") >= 0);

        for (size_t n = 0; n < 10; ++n) {
            while (bufferLock.test_and_set())
                ;

            assert(bufferRead == true);
            bufferRead = false;

            printf("Parent sending:");
            for (size_t b = 0; b < 5; ++b) {
                size_t number = rng();
                printf(" %08zu", number);
                buffer[b] = number;
            }
            printf("\n");

            bufferLock.clear();
            sleep(1);
        }
    } else {
        printf("In child\n");

        for (size_t n = 0; n < 10; ++n) {
            while (true) {
                while (bufferLock.test_and_set())
                    ;
                if (!bufferRead)
                    break;

                bufferLock.clear();
                for (volatile size_t i = 0; i < 10000; ++i)
                    ;
            }

            assert(bufferRead == false);
            bufferRead = true;

            printf("Child receiving:");
            for (size_t b = 0; b < 5; ++b)
                printf(" %08zu", buffer[b]);
            printf("\n");

            bufferLock.clear();
        }

        printf("Child should now crash (but not assert)\n");
        buffer[0] = 0;
        assert(false);
    }

    return 0;
}
