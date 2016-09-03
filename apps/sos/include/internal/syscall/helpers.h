#pragma once

namespace syscall {

inline boost::future<int> _returnNow(int result) {
    boost::promise<int> promise;
    promise.set_value(result);
    return promise.get_future();
}

template <typename T>
inline boost::future<int> sameException(boost::future<T> orig) {
    orig.get();
    boost::promise<int> promise;
    return promise.get_future();
};

inline std::system_error err(int syscall_no, const std::string& description="No reason given") {
    return std::system_error(syscall_no, std::system_category(), description);
}

}