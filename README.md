# QDPDK

Header-only library for fast prototyping network function.  
And some useful cores.
#### Can
- Make your own packet processing application.
- DPDK code fast prototyping.
#### Cannot
- Customize Ethdev API

### cores (./include/cores)
- [x] RoundRobin Distributor for Woker core
- [x] IPHash Distributor for Worker core
- [x] Agregator for Worker core
- [x] [RPPP](https://github.com/dotcom/RPPP) (Redundancy Protocol)
- [x] Fuse
- [x] Simple Packet Monitor
- [ ] Shaper
- [ ] Burst-dropper
- [ ] Packet Compressor

### interfeces (./include/qdpdk.hpp)

- [x] Physical port
- [x] Ring Queue (rte_ring)
- [ ] KNI

### Usage

#### Installation
0. Install DPDK(17.11.9).
1. qdpdk.hpp exists at `./include/qdpdk.hpp`
2. `#include "qdpdk.hpp"`

#### Example
```cpp
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
```

#### How to make your own cores
```cpp
#include "qdpdk.hpp"

template<class FROM, class TO>
class CoreSample{
protected:
    QDPDK::DeqInterface<FROM> from;
    QDPDK::EnqInterface<TO> to;
public:
    CoreSample(FROM deq, TO enq) : from(deq), to(enq){};
    // This is executed only once at the start of the core.
    void FirstCycle(){}
    // This is executed only once at the end of the core.
    void LastCycle(){}
    // This is executed in an infinite loop.
    void Cycle() {
        rte_mbuf *bufs[BURST_SIZE];

        // packet dequeue
        int nb_rx = from.Dequeue((rte_mbuf **)bufs, BURST_SIZE);

        if (unlikely(nb_rx == 0))
            return;

        // You can edit any packet.
        for (int n = 0; n < nb_rx; n++){
            // edit
        }

        // packet enqueue 
        to.Enqueue((rte_mbuf **)bufs, nb_rx);
    }
};
```

### vagrant
You can use vagrant to test a qdpdk.

1. `vagrant up`
2. `cd /vagrant`
3. `mkdir build && cd build`
4. `cmake ..`
5. `make`
6. `sudo ./test/all_tests`

This VM has two physical ports in addition to the main, and those two ports have been bound to DPDK. In addition, it is in promiscuous mode.

You can also test your core with this VM.
### vscode

1. `vagrant ssh-config >> ~/.ssh/config`

2. Install Remote SSH extension

3. Open