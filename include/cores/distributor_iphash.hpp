#pragma once
#include "qdpdk.hpp"
#define BURST_SIZE 32

template<class FROM, class TO0, class TO1>
class CoreDistributorIPHash{
protected:
    QDPDK::DeqInterface<FROM> from;
    QDPDK::EnqInterface<TO0> to0;
    QDPDK::EnqInterface<TO1> to1;
    bool switcher;
public:
    CoreDistributorIPHash(FROM from, TO0 to0, TO1 to1):
        from(from),
        to0(to0),
        to1(to1)
    {};

    void FirstCycle(){};
    void LastCycle(){};
    void Cycle() {
        rte_mbuf *bufs[BURST_SIZE];
        int nb_rx = from.Dequeue((rte_mbuf **)bufs, BURST_SIZE);

        if (unlikely(nb_rx == 0))
            return;

        for (int n = 0; n < nb_rx; n++){
            struct ipv4_hdr* ip = rte_pktmbuf_mtod_offset(
                bufs[n],
                struct ipv4_hdr*,
                sizeof(struct ether_hdr)
            );
            if((ip->src_addr)&0x00000001){
                to0.Enqueue((rte_mbuf **) &bufs[n], 1);
            }else{
                to1.Enqueue((rte_mbuf **) &bufs[n], 1);
            }
        }
    }  
};