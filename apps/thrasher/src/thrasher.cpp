#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

#include <boost/crc.hpp>

#include <cstdio>
#include <limits.h>
#include <stdint.h>

#ifndef PAGE_SIZE
    #define PAGE_SIZE 4096
#endif

std::mt19937 rng;
boost::crc_32_type crc;

constexpr const size_t ALLOCATION_PAGES = 256;
constexpr const size_t MAX_ALLOCATIONS = 1024 * 1024 * 1024 / ALLOCATION_PAGES / PAGE_SIZE;
constexpr const size_t THRASHES_PER_LOOP = 100000;

constexpr size_t pageId(size_t allocationN, size_t pageN) {
    return pageN + allocationN * ALLOCATION_PAGES;
}

void fillPage(uint8_t* page, size_t id) {
    size_t* data = reinterpret_cast<size_t*>(page);

    data[0] = id;
    for (size_t i = 1; i < PAGE_SIZE / sizeof(size_t) - 1; ++i)
        data[i] = rng();

    crc.reset();
    crc.process_bytes(data, PAGE_SIZE - sizeof(size_t));
    data[PAGE_SIZE / sizeof(size_t) - 1] = crc.checksum();
}

void checkPage(uint8_t* page, size_t id) {
    size_t* data = reinterpret_cast<size_t*>(page);

    if (data[0] != id)
        throw std::runtime_error("Page ID mismatch");

    crc.reset();
    crc.process_bytes(data, PAGE_SIZE - sizeof(size_t));
    if (data[PAGE_SIZE / sizeof(size_t) - 1] != crc.checksum())
        throw std::runtime_error("Page checksum mismatch");
}

int main() {
    std::fprintf(stderr, "Allocating as much as we can... ");
    std::vector<std::unique_ptr<uint8_t[]>> allocations;
    try {
        while (true) {
            allocations.emplace_back(
                std::make_unique<uint8_t[]>(ALLOCATION_PAGES * PAGE_SIZE)
            );

            if (allocations.size() == MAX_ALLOCATIONS)
                break;
        }
    } catch (const std::bad_alloc&) {}
    std::fprintf(stderr, "%zu MiB\n", ALLOCATION_PAGES * PAGE_SIZE * allocations.size() / 1024 / 1024);

    std::fprintf(stderr, "Filling with random data... ");
    for (size_t i = 0; i < allocations.size(); ++i)
        for (size_t j = 0; j < ALLOCATION_PAGES; ++j)
            fillPage(&allocations[i].get()[j * PAGE_SIZE], pageId(i, j));
    std::fprintf(stderr, "Done\n");

    std::uniform_int_distribution<size_t> allocationDist(0, allocations.size() - 1);
    std::uniform_int_distribution<size_t> pageDist(0, ALLOCATION_PAGES - 1);
    for (size_t loops = 0; ; ++loops) {
        std::fprintf(stderr, "Thrashed %zu pages\n", loops * THRASHES_PER_LOOP);
        for (size_t n = 0; n < THRASHES_PER_LOOP; ++n) {
            size_t i = allocationDist(rng);
            size_t j = pageDist(rng);

            uint8_t* page = &allocations[i].get()[j * PAGE_SIZE];
            size_t id = pageId(i, j);

            checkPage(page, id);
            fillPage(page, id);
        }
    }

    return 0;
}
