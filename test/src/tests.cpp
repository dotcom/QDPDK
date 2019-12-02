#include "gtest/gtest.h"
#include "qdpdk.hpp"
#include "cores/distributor_rr.hpp"
#include "cores/distributor_iphash.hpp"
#include "cores/agregator.hpp"
#include "cores/fuse.hpp"
#include <vector>
#include <array>
#include <bitset>
#include <cstring>

rte_ring *test_ring_in0;
rte_ring *test_ring_in1;
rte_ring *test_ring_out0;
rte_ring *test_ring_out1;
static struct rte_mempool*  mbuf_pool = NULL;

class CoreTest : public ::testing::Test {
protected:
    QDPDK::EnqInterface<rte_ring*> in0;
    QDPDK::EnqInterface<rte_ring*> in1;
    QDPDK::DeqInterface<rte_ring*> out0;
    QDPDK::DeqInterface<rte_ring*> out1;

    CoreTest():
        in0(test_ring_in0),
        in1(test_ring_in1),
        out0(test_ring_out0),
        out1(test_ring_out1)
    {}

    virtual void SetUp() {  
    };

    virtual void TearDown() {
    };

    template<typename T>
    struct simple_test{
    void operator()(){
    }
    };

    template<template<typename T> typename test_func>
    void tester(){
        test_func<int>()();
    }
};

TEST_F(CoreTest, test){
    tester<simple_test>();
}

TEST_F(CoreTest, distributor_rr_test){
    auto core = CoreDistributorRR(test_ring_in0, test_ring_out0, test_ring_out1);

    auto pkt = rte_pktmbuf_alloc(mbuf_pool);
    EXPECT_EQ(pkt!=NULL,true);

    rte_mbuf *buf[1];
    buf[0] = pkt;

    // First Enqueue
    EXPECT_EQ(in0.Enqueue((rte_mbuf **)buf, 1), 1);
    core.Cycle();
    EXPECT_EQ(out0.Dequeue((rte_mbuf **)buf, 1), 1);

    // Second Enqueue
    EXPECT_EQ(in0.Enqueue((rte_mbuf **)buf, 1), 1);
    core.Cycle();
    EXPECT_EQ(out1.Dequeue((rte_mbuf **)buf, 1), 1);

    // Third Enqueue
    EXPECT_EQ(in0.Enqueue((rte_mbuf **)buf, 1), 1);
    core.Cycle();
    EXPECT_EQ(out0.Dequeue((rte_mbuf **)buf, 1), 1);

    rte_pktmbuf_free(pkt);
}

TEST_F(CoreTest, distributor_iphash_test){
    auto core = CoreDistributorIPHash(test_ring_in0, test_ring_out0, test_ring_out1);

    auto pkt = rte_pktmbuf_alloc(mbuf_pool);
    EXPECT_EQ(pkt!=NULL,true);

    struct ipv4_hdr* ip = rte_pktmbuf_mtod_offset(
        pkt,
        struct ipv4_hdr*,
        sizeof(struct ether_hdr)
    );

    rte_mbuf *buf[1];
    buf[0] = pkt;

    ip->src_addr = 0x01010101;

    // First Enqueue
    EXPECT_EQ(in0.Enqueue((rte_mbuf **)buf, 1), 1);
    core.Cycle();
    EXPECT_EQ(out0.Dequeue((rte_mbuf **)buf, 1), 1);

    ip->src_addr = 0x01010100;

    // Second Enqueue
    EXPECT_EQ(in0.Enqueue((rte_mbuf **)buf, 1), 1);
    core.Cycle();
    EXPECT_EQ(out1.Dequeue((rte_mbuf **)buf, 1), 1);

    ip->src_addr = 0x01010100;

    // Third Enqueue
    EXPECT_EQ(in0.Enqueue((rte_mbuf **)buf, 1), 1);
    core.Cycle();
    EXPECT_EQ(out1.Dequeue((rte_mbuf **)buf, 1), 1);

    ip->src_addr = 0x01010101;

    // Fourth Enqueue
    EXPECT_EQ(in0.Enqueue((rte_mbuf **)buf, 1), 1);
    core.Cycle();
    EXPECT_EQ(out0.Dequeue((rte_mbuf **)buf, 1), 1);


    rte_pktmbuf_free(pkt);
}

TEST_F(CoreTest, agregator){
    auto core = CoreAgregator(test_ring_in0, test_ring_in1, test_ring_out0);

    auto pkt = rte_pktmbuf_alloc(mbuf_pool);
    EXPECT_EQ(pkt!=NULL,true);

    rte_mbuf *buf[2];
    buf[0] = pkt;

    // First Enqueue
    EXPECT_EQ(in0.Enqueue((rte_mbuf **)buf, 1), 1);
    core.Cycle();
    EXPECT_EQ(out0.Dequeue((rte_mbuf **)buf, 1), 1);

    // Second Enqueue
    EXPECT_EQ(in1.Enqueue((rte_mbuf **)buf, 1), 1);
    core.Cycle();
    EXPECT_EQ(out0.Dequeue((rte_mbuf **)buf, 1), 1);

    // Third Enqueue
    EXPECT_EQ(in0.Enqueue((rte_mbuf **)buf, 1), 1);
    EXPECT_EQ(in0.Enqueue((rte_mbuf **)buf, 1), 1);
    core.Cycle();
    EXPECT_EQ(out0.Dequeue((rte_mbuf **)buf, 2), 2);


    rte_pktmbuf_free(pkt);
}

int main(int argc, char *argv[]) {
    int port_count = 0;
    auto app = QDPDK::App(port_count, argc, argv);
    int socket_id = 0;

    test_ring_in0 = app.CreateRing("ring0", 1024, socket_id);
    test_ring_in1 = app.CreateRing("ring1", 1024, socket_id);
    test_ring_out0 = app.CreateRing("ring2", 1024, socket_id);
    test_ring_out1 = app.CreateRing("ring3", 1024, socket_id);

    if ((mbuf_pool = rte_pktmbuf_pool_create("test_mbuf_pool", MBUF_PER_POOL,
                                            MBUF_POOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
                                            rte_socket_id())) == NULL){
        rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");
    }

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
