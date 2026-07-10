#include "EventScheduler.hpp"

namespace pse {

void EventSched::add_event(FuncType func, u64 t){
    m_pq.emplace(m_clk + t, func);
}

void EventSched::tick(){
    while (m_pq.top().time <= m_clk){{

    }
}

}

