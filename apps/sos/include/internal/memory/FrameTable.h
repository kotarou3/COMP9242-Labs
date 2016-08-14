#pragma once

extern "C" {
    #include <sel4/sel4.h>
}

namespace memory {

class Page;

namespace FrameTable {
    Page allocate();
}

class Page {
    public:
        Page(const Page&) = delete;
        Page& operator=(const Page&) = delete;

        Page(Page&& other);
        Page& operator=(Page&& other);

        seL4_ARM_Page getCap() const noexcept {return _cap;};

    private:
        Page() = default;

        seL4_ARM_Page _cap;
};

}
