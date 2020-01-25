#pragma once
#include "qdpdk.hpp"
#define BURST_SIZE 32

template<class FROM0, class FROM1, class TO>
class CoreAgregator{
protected:
    QDPDK::DeqInterface<FROM0> from0;
    QDPDK::DeqInterface<FROM1> from1;
    QDPDK::EnqInterface<TO> to;
public:
    CoreAgregator(FROM0 from0, FROM1 from1, TO to):
        from0(from0),
        from1(from1),
        to(to)
    {}

    void FirstCycle(){}
    void LastCycle(){}
    void Cycle() {
        rte_mbuf *bufs0[BURST_SIZE];
        rte_mbuf *bufs1[BURST_SIZE];
        int nb_rx0 = from0.Dequeue((rte_mbuf **) bufs0, BURST_SIZE);
        int nb_rx1 = from1.Dequeue((rte_mbuf **) bufs1, BURST_SIZE);

        if (unlikely(nb_rx0 == 0 && nb_rx1 == 0))
            return;
        else if (unlikely(nb_rx1 == 0))
            to.Enqueue((rte_mbuf **) bufs0, nb_rx0);
        else if (unlikely(nb_rx0 == 0))
            to.Enqueue((rte_mbuf **) bufs1, nb_rx1);
        else{
            to.Enqueue((rte_mbuf **) bufs0, nb_rx0);
            to.Enqueue((rte_mbuf **) bufs1, nb_rx1);
        }
    }
};
