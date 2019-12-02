#pragma once
#include "qdpdk.hpp"
#define BURST_SIZE 32

template<class FROM, class TO0, class TO1>
class CoreDistributorRR{
protected:
    QDPDK::DeqInterface<FROM> from;
    QDPDK::EnqInterface<TO0> to0;
    QDPDK::EnqInterface<TO1> to1;
    bool switcher;
public:
    CoreDistributorRR(FROM from, TO0 to0, TO1 to1):
        from(from),
        to0(to0),
        to1(to1),
        switcher(true)
    {};

    void FirstCycle(){};
    void LastCycle(){};
    void Cycle() {
        rte_mbuf *bufs[BURST_SIZE];
        int nb_rx = from.Dequeue((rte_mbuf **)bufs, BURST_SIZE);

        if (unlikely(nb_rx == 0))
            return;

        switch(switcher){
        case true:
            to0.Enqueue((rte_mbuf **)bufs, nb_rx);
            switcher = false;
            break;
        case false:
            to1.Enqueue((rte_mbuf **)bufs, nb_rx);
            switcher = true;
            break;
        }
    }  
};