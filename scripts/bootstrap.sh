# install
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y sudo  wget libpcap-dev linux-headers-`uname -r` python build-essential libnuma-dev cmake make git vim iproute2 ethtool tcpdump numactl

# dpdk
export DPDK_VER=17.11.9
export RTE_TARGET=x86_64-native-linuxapp-gcc
export RTE_SDK=/usr/local/src/dpdk-stable-$DPDK_VER
cd /usr/local/src
wget https://fast.dpdk.org/rel/dpdk-$DPDK_VER.tar.xz && tar -xf dpdk-$DPDK_VER.tar.xz && rm dpdk-$DPDK_VER.tar.xz
chown -R vagrant:vagrant $RTE_SDK
cd $RTE_SDK
make install T=$RTE_TARGET
echo "export DPDK_VER="${DPDK_VER} >> /home/vagrant/.bashrc
echo "export RTE_TARGET="${RTE_TARGET} >> /home/vagrant/.bashrc
echo "export RTE_SDK="${RTE_SDK} >> /home/vagrant/.bashrc

# hugepage
mkdir -p /mnt/huge
