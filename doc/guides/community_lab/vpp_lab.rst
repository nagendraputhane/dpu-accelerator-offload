
..  SPDX-License-Identifier: Marvell-MIT
    Copyright (c) 2024 Marvell.

Running VPP applications
========================

Prerequisites
-------------
a. Linux booted on Host and DPU

b. Login to your docker on host and DPU

c. Bind the pktio and crypto devices to vfio-pci

.. code-block:: console

   source dao-env.sh


DAO Environment Setup
---------------------
Following step is required to run only once after the first login to docker

.. code-block:: console

   ~# source /dao-env.sh


L3 Router
---------
a. start vpp with config file at /etc/vpp/pktio_startup.conf

.. code-block:: console

   ~# vpp -c /etc/vpp/pktio_startup.conf


b. start vppctl command on console

.. code-block:: console

   ~# vppctl

   vpp# set int state eth0 up
   vpp# set int state eth1 up
   vpp# set int ip address eth0 10.29.10.1/24
   vpp# set int ip address eth1 10.29.20.2/24
   vpp# set ip neighbor eth0  10.29.10.10 00:00:00:01:01:01
   vpp# set ip neighbor eth1  10.29.20.20 00:00:00:02:01:01
   vpp# ip route add 10.29.10.10/24 via eth0
   vpp# ip route add 10.29.20.20/24 via eth1
   vpp# show int
   vpp# trace add eth0-rx 5

c. On host x86 machine, configure the IPs of the interfaces to send traffic

.. code-block:: console

   ~# ifconfig intf1  10.29.10.10/24
   ~# ifconfig intf2  10.29.20.20/24

d. Run tshark capture on intf2

.. code-block:: console

   ~# tshark -i intf2 -Y "udp" -V

e. Run scapy and send traffic

.. code-block:: console

   ~# scapy

    >>> sendp(Ether(dst="ba:7a:5a:ae:c7:ab",src="00:00:00:01:01:01")/IP(src="10.29.10.10",dst="10.29.20.20",len=60)/UDP(dport=4000,sport=4000,len=40)/Raw(RandString(size=32)), iface="intf1", return_packets=True, count=100)

f. Observe the traffic on tshark console on x86 host

g. On VPP console check the graph walk

.. code-block:: console

   vpp# show trace


VPP as IPsec Tunnel Originator
------------------------------
h. start vpp with config file at /etc/vpp/pktio_startup.conf

.. code-block:: console

   ~# vpp -c /etc/vpp/crypto_startup.conf

i. start vppctl command on console

.. code-block:: console

   ~# vppctl

   vpp# set int ip address eth0 10.29.10.1/24
   vpp# set int state eth0 up
   vpp# set ip neighbor eth0 10.29.10.10 00:00:00:01:01:01
   vpp# set int promiscuous on eth0
   vpp# set int ip address eth1 192.168.1.1/24
   vpp# set ip neighbor eth1 192.168.1.2 00:00:00:02:01:01
   vpp# set int state eth1 up
   vpp# set int promiscuous on eth1
   vpp# set ipsec async mode on
   vpp# ipsec itf create
   vpp# ipsec sa add 10 spi 1001 esp crypto-key 4a506a794f574265564551694d653768 crypto-alg aes-gcm-128 tunnel src 192.168.1.1 dst 192.168.1.2 esp
   vpp# ipsec sa add 20 spi 2001 inbound crypto-alg aes-gcm-128 crypto-key 4d4662776d4d55747559767176596965 tunnel src 192.168.1.2 dst 192.168.1.1 esp
   vpp# ipsec tunnel protect sa-out 10 ipsec0
   vpp# set int state ipsec0 up
   vpp# set interface unnum ipsec0 use eth1
   vpp# ip route add 10.29.20.20/24 via ipsec0
   vpp# show int
   vpp# pcap trace tx  intfc eth1 max 100 file outbound_enc.pcap
   vpp# trace add eth0-rx 5

j. On host x86 machine, configure the IPs of the interfaces to send traffic

.. code-block:: console

   ~# ifconfig intf1  10.29.10.10/24
   ~# ifconfig intf2  10.29.20.20/24

k. Run tshark capture on intf2

.. code-block:: console

   ~# tshark -i intf2 -Y "esp" -V

l. Run scapy and send traffic

.. code-block:: console

   ~# scapy

    >>> sendp(Ether(dst="ba:7a:5a:ae:c7:ab",src="00:00:00:01:01:01")/IP(src="10.29.10.10",dst="10.29.20.20",len=60)/UDP(dport=4000,sport=4000,len=40)/Raw(RandString(size=32)), iface="intf1", return_packets=True, count=5)

m. Observe the ESP traffic on tshark console on x86 host
   ~# tshark -i intf2 -Y "esp" -V

n. On VPP console check the graph walk
   vpp# show trace

