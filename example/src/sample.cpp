#include "qdpdk.hpp"
#include "cores/distributor_rr.hpp"
#include "cores/agregator.hpp"
#include "cores/pdump_log.hpp"
/*

    NIC RX                                                NIC TX
      |                                                     /|\
     \|/          /--ring0--> CoreWorker0 --ring2--\         |
CoreDistributor -|                                  |-> CoreAgregator
                  \--ring1--> CoreWorker1 --ring3--/
*/

int main(int argc, char *argv[]){
    int port_count = 2;
    auto app = QDPDK::App(port_count, argc, argv);

    // CPU socket
    int socket_id = 0;
    // Create some rings
    rte_ring* ring0 = app.CreateRing("ring0", 1024, socket_id);
    rte_ring* ring1 = app.CreateRing("ring1", 1024, socket_id);
    rte_ring* ring2 = app.CreateRing("ring2", 1024, socket_id);
    rte_ring* ring3 = app.CreateRing("ring3", 1024, socket_id);
    // Set port numbers
    int port0 = 0;
    int port1 = 1;

    // Instanciate Cores
    auto core_rx = CoreDistributorRR(port0, ring0, ring1);
    auto worker0 = CorePdumpLog(ring0, ring2);
    auto worker1 = CorePdumpLog(ring1, ring3);
    auto core_tx = CoreAgregator(ring2, ring3, port1);

    // Set Cores to app
    app.SetCore(&core_rx);
    app.SetCore(&worker0);
    app.SetCore(&worker1);
    app.SetCore(&core_tx);
    
    app.Run();
}