#pragma once
#include <atomic>
#include <memory>
#include <utility>

namespace theater {
// Readers retain a complete immutable value. Publishing never holds a lock
// across native game work, file IO, or a reader's copy of the value.
template<class T> class published_value {
    std::atomic<std::shared_ptr<const T>> value{std::make_shared<const T>()};
public:
    void publish(T next){value.store(std::make_shared<const T>(std::move(next)),std::memory_order_release);}
    T read() const {return *value.load(std::memory_order_acquire);}
};
}
