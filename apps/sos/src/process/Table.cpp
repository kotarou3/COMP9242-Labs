#include <limits>

#include "internal/process/Table.h"

namespace process {

pid_t ThreadTable::insert(std::shared_ptr<Thread> thread) {
    if (_table.size() >= MAX_TID - MIN_TID + 1)
        throw std::system_error(ENOMEM, std::system_category(), "Too many threads");

    while (_table.count(_nextFree) > 0)
        if (_nextFree == MAX_TID)
            _nextFree = MIN_TID;
        else
            ++_nextFree;

    pid_t tid = _nextFree;
    _table[tid] = std::move(thread);

    if (_nextFree == MAX_TID)
        _nextFree = MIN_TID;
    else
        ++_nextFree;

    return tid;
}

bool ThreadTable::erase(pid_t tid) noexcept {
    return _table.erase(tid);
}

std::shared_ptr<Thread> ThreadTable::get(pid_t tid) const {
    try {
        return _table.at(tid);
    } catch (const std::out_of_range&) {
        std::throw_with_nested(std::system_error(ESRCH, std::system_category()));
    }
}


ThreadTable& ThreadTable::get() noexcept {
    static ThreadTable _singleton;
    return _singleton;
}

}
