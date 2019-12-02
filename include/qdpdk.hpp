#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <signal.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_log.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_icmp.h>
#include <string>
#include <vector>
#include <fstream>
#include <istream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <atomic>
#define MBUF_PER_POOL 65535
#define MBUF_POOL_CACHE_SIZE 250
#define RX_RING_SIZE 512
#define TX_RING_SIZE 512

namespace QDPDK {
    enum RET{
        OK = 0,
        FAIL,
    };

    template<typename T>
    class DeqInterface{
    protected:
        T queue;
    public:
        DeqInterface(T q):queue(q){}
        uint16_t Dequeue(rte_mbuf** bufs, size_t size) = delete;
    };

    template<>
    uint16_t DeqInterface<int>::Dequeue(rte_mbuf** bufs, size_t size){
        return rte_eth_rx_burst(queue, 0, bufs, size);
    }

    template<>
    uint16_t DeqInterface<rte_ring*>::Dequeue(rte_mbuf** bufs, size_t size){
        return rte_ring_sc_dequeue_burst(queue, (void**)bufs, size, NULL);
    };

    template<typename T>
    class EnqInterface{
    protected:
        T queue;
    public:
        EnqInterface(T q):queue(q){}
        uint16_t Enqueue(rte_mbuf** bufs, size_t n) = delete;
    };

    template<>
    uint16_t EnqInterface<int>::Enqueue(rte_mbuf** bufs, size_t n){
        uint16_t nb_tx = rte_eth_tx_burst(queue, 0, bufs, n);
        if (unlikely(nb_tx < n)) {
            for (auto buf = nb_tx; buf < n; buf++)
                rte_pktmbuf_free(bufs[buf]);
        }
        return nb_tx;
    };

    template<>
    uint16_t EnqInterface<rte_ring*>::Enqueue(rte_mbuf** bufs, size_t n){
        uint16_t nb_tx = rte_ring_sp_enqueue_burst(queue, (void**)bufs, n, NULL);
        if (unlikely(nb_tx < n)) {
            for (auto buf = nb_tx; buf < n; buf++)
                rte_pktmbuf_free(bufs[buf]);
        }
        return nb_tx;
    };

    class App{
    public:
        static std::atomic<bool> quit;
        static std::atomic<bool> start;
        static struct rte_eth_conf port_conf;
    protected:
        rte_mempool *mbuf_pool;
        int mempool_size;
        int num_ports;
        int core_cnt;
    public:
        App(int num_ports, int argc, char *argv[]) : num_ports(num_ports){ AppInit(argc, argv);};
        RET Run();
        template<class C>
        RET SetCore(C*);
        rte_ring* CreateRing(const char *name, unsigned count, int socket_id, unsigned flags);
        static void Signal_handler(int signum);
    private:
        RET PortInit(int);
        RET AppInit(int, char**);
        template<class C>
        static RET CoreLoop(C*);
        unsigned int get_available_lcore_id();
    };
    std::atomic<bool>  App::quit = false;
    std::atomic<bool>  App::start = false;
    struct rte_eth_conf App::port_conf;

    void App::Signal_handler(int signum){
        if (signum == SIGINT || signum == SIGTERM) {
            quit = true;
        }
    }

    RET App::PortInit(int port)
    {
        const uint16_t rx_rings = 1, tx_rings = 1;
        uint16_t nb_rxd = RX_RING_SIZE;
        uint16_t nb_txd = TX_RING_SIZE;
        int retval;
        uint16_t q;

        port_conf.rxmode.max_rx_pkt_len = ETHER_MAX_LEN;

        if (port >= rte_eth_dev_count())
            return FAIL;

        retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
        if (retval != 0)
            return FAIL;

        retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
        if (retval != 0)
            return FAIL;

        for (q = 0; q < rx_rings; q++) {
            retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
                                            rte_eth_dev_socket_id(port), NULL, mbuf_pool);
            if (retval < 0)
                return FAIL;
        }

        for (q = 0; q < tx_rings; q++) {
            retval = rte_eth_tx_queue_setup(port, q, nb_txd,
                                            rte_eth_dev_socket_id(port), NULL);
            if (retval < 0)
                return FAIL;
        }

        retval = rte_eth_dev_start(port);
        if (retval < 0)
            return FAIL;

        struct ether_addr addr;
        rte_eth_macaddr_get(port, &addr);
        printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
        " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
                port,
                addr.addr_bytes[0], addr.addr_bytes[1],
                addr.addr_bytes[2], addr.addr_bytes[3],
                addr.addr_bytes[4], addr.addr_bytes[5]);

        rte_eth_promiscuous_enable(port);

        return OK;
    }

    RET App::AppInit(int argc, char *argv[]) {
        int ret = rte_eal_init(argc, argv);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

        argc -= ret;
        argv += ret;

        int nb_ports = rte_eth_dev_count();
        if (nb_ports < num_ports)
            rte_exit(EXIT_FAILURE, "Error: fewer ports than the bind ports.\n");

        mbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", MBUF_PER_POOL,
                                            MBUF_POOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
                                            rte_socket_id());

        if (mbuf_pool == NULL)
            rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

        for (int portid = 0; portid < nb_ports; portid++)
            if (PortInit(portid) != 0)
                rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n", portid);

        core_cnt = rte_lcore_count();

        signal(SIGINT, Signal_handler);
        signal(SIGTERM, Signal_handler);
        return OK;
    }

    template<class C>
    RET App::SetCore(C *core) {
        rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "SETTING CORE.\n");
        if (--core_cnt < 0)
            rte_exit(EXIT_FAILURE, "Error: fewer cores than the set cores.\n");
        rte_eal_remote_launch((lcore_function_t *) CoreLoop<C>, core, get_available_lcore_id());
        return OK;
    }

    rte_ring* App::CreateRing(const char *name, unsigned count, int socket_id, unsigned flags=RING_F_SP_ENQ|RING_F_SC_DEQ) {
        rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "CREATING RING.\n");
        return rte_ring_create(name, count, socket_id, flags);
    }

    template<class C>
    RET App::CoreLoop(C *core){
        while(not start) rte_delay_ms(10);

        rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "START lcore:%d\n", rte_lcore_id());
        core->FirstCycle();
        while(not quit){
            core->Cycle();
        }
        core->LastCycle();
        rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "FINISH lcore:%d\n", rte_lcore_id());
        return OK;
    }

    RET App::Run() {
        rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "START master\n");
        start = true;
        rte_eal_mp_wait_lcore();
        rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER3, "FINISH master\n");
        return OK;
    }

    unsigned int App::get_available_lcore_id() {
        static bool assigned[RTE_MAX_LCORE] = {false,};
        for (uint i = RTE_MAX_LCORE - 1; i >= 0; i--){
            if (!rte_lcore_is_enabled(i)){
                continue;
            }
            if (i == rte_get_master_lcore()){
                continue;
            }
            if (assigned[i]){
                continue;
            }
            assigned[i] = true;

            return i;
        }
        return 0;
    }
}

