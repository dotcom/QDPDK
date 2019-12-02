#pragma once
#include "qdpdk.hpp"
#define BURST_SIZE 32

template<class FROM, class TO>
class CorePdumpLog{
protected:
    QDPDK::DeqInterface<FROM> from;
    QDPDK::EnqInterface<TO> to;
public:
    CorePdumpLog(FROM deq, TO enq) : from(deq), to(enq){};
    void FirstCycle(){}
    void LastCycle(){}
    void Cycle() {
        rte_mbuf *bufs[BURST_SIZE];

        int nb_rx = from.Dequeue((rte_mbuf **)bufs, BURST_SIZE);

        if (unlikely(nb_rx == 0))
            return;

        for (int n = 0; n < nb_rx; n++){
            int len = bufs[n]->data_len;
            auto pkt = rte_pktmbuf_mtod(bufs[n], char*);
            char str[ETHER_MAX_LEN*5] = "";

            for (int i=0;i<len;i++){
                sprintf(str, "%s 0x%02x", str, 0x000000ff & pkt[i]);
            }
            rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "==== PDUMP LOG ==== %u\n %s\n", rte_lcore_id(), str);
        }

        to.Enqueue((rte_mbuf **)bufs, nb_rx);
    }
};