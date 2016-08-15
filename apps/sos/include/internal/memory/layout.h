#pragma once

namespace memory {

using vaddr_t = size_t;

constexpr const vaddr_t MMAP_START       = 0x10000000;
constexpr const vaddr_t MMAP_END         = 0xd0000000;
constexpr const vaddr_t MMAP_STACK_START = MMAP_END;
constexpr const vaddr_t MMAP_STACK_END   = 0xe0000000;

// seL4 reserves 0xe0000000 onwards
constexpr const vaddr_t KERNEL_START = 0xe0000000;

constexpr const size_t MMAP_RAND_ATTEMPTS = 4;

constexpr const size_t STACK_PAGES = 256; // 1 MB

}
