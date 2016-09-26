#pragma once

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>

#define syscall(...) _syscall(__VA_ARGS__)
// Includes missing from inline_executor.hpp
#include <boost/thread/thread.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>

#include <boost/thread/executors/inline_executor.hpp>
#include <boost/thread/future.hpp>
#undef syscall

#define then(...) then(::async::asyncExecutor, __VA_ARGS__)

namespace async {

extern boost::inline_executor asyncExecutor;

using boost::promise;
using boost::future;
using boost::make_ready_future;
using boost::make_exceptional_future;

template <typename InputIt>
future<std::vector<typename std::iterator_traits<InputIt>::value_type>>
    when_all(InputIt first, InputIt last) {
    using Result = std::vector<typename std::iterator_traits<InputIt>::value_type>;

    auto allResults = std::make_shared<Result>(std::distance(first, last));
    auto finishedCount = std::make_shared<size_t>(0);
    size_t targetCount = allResults->size();

    auto allResultsPromise = std::make_shared<promise<Result>>();

    auto resultIt = allResults->begin();
    for (InputIt it = first; it != last; ++it, ++resultIt) {
        it->then([=](auto result) {
            *resultIt = std::move(result);
            if (++*finishedCount == targetCount)
                allResultsPromise->set_value(std::move(*allResults));
        });
    }

    return allResultsPromise->get_future();
}

template <size_t f, typename Tuple>
void _when_all_helper(
    const std::shared_ptr<Tuple>& /*tuple*/,
    const std::shared_ptr<size_t>& /*finishedCount*/,
    const std::shared_ptr<promise<Tuple>>& /*tuplePromise*/) {
    static_assert(f == std::tuple_size<Tuple>::value, "Tuple is of wrong size");
}

template <size_t f = 0, typename Tuple, typename Future0, typename ...Futures>
void _when_all_helper(
    std::shared_ptr<Tuple> tuple,
    std::shared_ptr<size_t> finishedCount,
    std::shared_ptr<promise<Tuple>> tuplePromise,
    Future0&& future0,
    Futures&& ...futures) {
    future0.then([tuple, finishedCount, tuplePromise](auto result) {
        std::get<f>(*tuple) = std::move(result);
        if (++*finishedCount == std::tuple_size<Tuple>::value)
            tuplePromise->set_value(std::move(*tuple));
    });

    _when_all_helper<f + 1>(tuple, finishedCount, tuplePromise, futures...);
}

template <typename ...Futures>
future<std::tuple<std::decay_t<Futures>...>> when_all(Futures&& ...futures) {
    using Result = std::tuple<std::decay_t<Futures>...>;

    auto tuple = std::make_shared<Result>();
    auto finishedCount = std::make_shared<size_t>(0);
    auto tuplePromise = std::make_shared<promise<Result>>();

    _when_all_helper(tuple, finishedCount, tuplePromise, futures...);
    return tuplePromise->get_future();
}

}
