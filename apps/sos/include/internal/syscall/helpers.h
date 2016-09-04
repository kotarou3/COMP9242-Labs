#pragma once

namespace syscall {

inline boost::future<int> _returnNow(int result) {
    boost::promise<int> promise;
    promise.set_value(result);
    return promise.get_future();
}

template <typename T>
inline std::exception_ptr getException(boost::future<T>& future) {
    try {
        future.get();
    } catch (...) {
        return std::current_exception();
    }
    return nullptr;
};

inline std::system_error err(int syscall_no, const std::string& description="No reason given") {
    return std::system_error(syscall_no, std::system_category(), description);
}

}