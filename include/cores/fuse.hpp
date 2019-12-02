#pragma once
#include "qdpdk.hpp"
#include <chrono>
#define BURST_SIZE 32

template<class FROM, class TO>
class CoreFuse{
private:
    uint64_t byte_counter;
    uint64_t before_cycles;
    uint64_t bps_limit;
    bool stop;
    QDPDK::DeqInterface<FROM> from;
    QDPDK::EnqInterface<TO> to;

public:
    CoreFuse(FROM deq, TO enq, uint64_t bps_limit):
        byte_counter(0),
        before_cycles(0),
        bps_limit(bps_limit),
        stop(false),
        from(deq),
        to(enq)
    {}

    void FirstCycle(){};
    void LastCycle(){};
    void Cycle() {
        if(not stop){
            rte_mbuf *bufs[BURST_SIZE];

            int nb_rx = from.Dequeue((rte_mbuf **)bufs, BURST_SIZE);

            if (unlikely(nb_rx == 0))
                return;

            for (int n = 0; n < nb_rx; n++){
                byte_counter += bufs[n]->data_len;
            }

            auto now_cycles = rte_get_timer_cycles();
            auto diff = now_cycles - before_cycles;

            if(diff >  rte_get_timer_hz()){
                if(byte_counter > bps_limit){
                    stop = true;
                }
                byte_counter = 0;
                before_cycles = now_cycles;
            }

            to.Enqueue((rte_mbuf **)bufs, nb_rx);
        }
    }
};