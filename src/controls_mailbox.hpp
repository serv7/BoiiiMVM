#pragma once
#include "backend.hpp"
#include <mutex>
#include <optional>

namespace theater {
// This lock protects only small value copies; no native work or file IO runs
// under it. Native CFG updates merge the latest UI request before their fields.
class controls_mailbox {
    std::mutex mutex;
    options visible;
    std::optional<options> pending;
public:
    options read(){std::lock_guard lock(mutex);return visible;}
    void initialize(options value){std::lock_guard lock(mutex);visible=value;pending.reset();}
    void request(options value){std::lock_guard lock(mutex);visible=value;pending=value;}
    std::optional<options> take(){std::lock_guard lock(mutex);auto value=pending;pending.reset();return value;}
    template<class Apply> options merge(options value,Apply apply){
        std::lock_guard lock(mutex);
        if(pending){value=*pending;pending.reset();}
        apply(value);visible=value;return value;
    }
};
}
