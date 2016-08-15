#include "internal/RandomDevice.h"

RandomDevice& RandomDevice::getSingleton() noexcept {
    static RandomDevice singleton;
    return singleton;
}
