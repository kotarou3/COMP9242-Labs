#pragma once

namespace memory {

using vaddr_t = size_t;

constexpr const size_t SOS_INIT_AREA_SIZE = 0x400000;
constexpr const vaddr_t SOS_BRK_START = 0x900000;
constexpr const vaddr_t SOS_BRK_END = SOS_BRK_START + 50 * 1024 * 1024;

constexpr const vaddr_t MMAP_START       = 0x10000000;
constexpr const vaddr_t MMAP_END         = 0xd0000000;
constexpr const vaddr_t MMAP_STACK_START = MMAP_END;
constexpr const vaddr_t MMAP_STACK_END   = 0xe0000000;

// seL4 reserves 0xe0000000 onwards
constexpr const vaddr_t KERNEL_START = 0xe0000000;

constexpr const size_t MMAP_RAND_ATTEMPTS = 4;

constexpr const size_t STACK_PAGES = 256; // 1 MB

static_assert(SOS_BRK_END < MMAP_START, "SOS brk overlaps with mmap area");

}
