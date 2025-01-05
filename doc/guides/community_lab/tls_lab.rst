
..  SPDX-License-Identifier: Marvell-MIT
    Copyright (c) 2024 Marvell.

Running TLS applications
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


OpenSSL Speed Application
-------------------------
a. Run speed application without engine

.. code-block:: console

  ~# cd /usr/lib/cn10k/openssl-1.1.1q/bin
  ~# export LD_LIBRARY_PATH=/usr/lib/cn10k/openssl-1.1.1q/lib/
  ~# ./openssl speed -elapsed -async_jobs +24 -evp aes-256-gcm
  ~# ./openssl speed -elapsed rsa2048

b. Run speed application with engine

.. code-block:: console

  ~# OPENSSL_CONF=/opt/openssl.cnf ./openssl speed -elapsed -async_jobs +24 -evp aes-256-gcm
  ~# OPENSSL_CONF=/opt/openssl.cnf ./openssl speed -elapsed -async_jobs +24 rsa2048


Openssl server and client
-------------------------
c. Run openssl s_server on DPU

.. code-block:: console

  ~# OPENSSL_CONF=/opt/openssl.cnf ./openssl s_server -key certs/server.key.pem -cert certs/server.crt.pem -accept 4433 -tls1_2

d. Run openssl s_client on x86 host machine

.. code-block:: console

  ~# openssl s_client -connect <DUT_IP>:4433 -tls1_2

  <DUT_IP> is the IP of s_server on DPU
