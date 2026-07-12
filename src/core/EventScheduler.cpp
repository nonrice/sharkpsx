#include "EventScheduler.hpp"

namespace pse {

void EventSched::add(FuncType func, u64 t){
    m_pq.emplace(m_clk + t, func);
}

void EventSched::tick(){
    while (!m_pq.empty() && m_pq.top().time <= m_clk){
        Event e = m_pq.top();
        m_pq.pop();

        e.func();
    }

    m_clk += 1;
}

}

