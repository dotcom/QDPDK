#pragma once
#include "qdpdk.hpp"
#include <atomic>
#define BURST_SIZE 32

template<class FROM, class TO>
class CoreCounter{
public:
    std::atomic<uint64_t> packet_counter;
    std::atomic<uint64_t> byte_counter;
protected:
    QDPDK::DeqInterface<FROM> from;
    QDPDK::EnqInterface<TO> to;

public:
    CoreCounter(FROM deq, TO enq) : packet_counter(0), byte_counter(0), from(deq), to(enq){}

    void FirstCycle(){};
    void LastCycle(){};
    void Cycle() {
        rte_mbuf *bufs[BURST_SIZE];

        int nb_rx = from.Dequeue((rte_mbuf **)bufs, BURST_SIZE);

        if (unlikely(nb_rx == 0))
            return;

        for (int n = 0; n < nb_rx; n++){
            packet_counter++;
            byte_counter += bufs[n]->data_len;
        }

        to.Enqueue((rte_mbuf **)bufs, nb_rx);
    }
};