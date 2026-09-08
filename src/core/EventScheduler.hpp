#pragma once

#include <functional>
#include <queue>

#include "types.hpp"

namespace pse {

class EventSched {
public:
    using FuncType = std::function<void(void)>;

    void add(FuncType func, u64 t);
    void tick();


private:
    struct Event {
        u64 time;
        FuncType func;

        bool operator>(const Event& o) const {
            return time > o.time;
        }
    };

    u64 m_clk{ 0 };
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> m_pq;
};

}
