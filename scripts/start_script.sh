export DPDK_VER=17.11.9
export RTE_TARGET=x86_64-native-linuxapp-gcc
export RTE_SDK=/usr/local/src/dpdk-stable-$DPDK_VER
(mount | grep hugetlbfs) > /dev/null || mount -t hugetlbfs nodev /mnt/huge
echo 512 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
modprobe uio
insmod $RTE_SDK/$RTE_TARGET/kmod/igb_uio.ko

ifconfig eth1 down
$RTE_SDK/usertools/dpdk-devbind.py -b igb_uio 0000:00:08.0
ifconfig eth2 down
$RTE_SDK/usertools/dpdk-devbind.py -b igb_uio 0000:00:09.0
