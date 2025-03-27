..  SPDX-License-Identifier: Marvell-MIT
    Copyright (c) 2024 Marvell.

DAO Release 25.01.0
===================

New Features
------------

* **Applications**

  * *DPDK based OpenSSL Engine*

    Added support for OpenSSL 3.x version.
    Bug fixes in ECDSA verify operations.

  * *OVS*

    Solution migrated to OVS-3.4.1.
    Integration with Flow library - enhancing scope to manage large no of flows.

* **Infrastructure**

  * New debian packages for DAO solutions

    - ML Models
    - Snort

Debian Pakages List
-------------------

- **DAO**
  - **dao-cn10k**
  - **Version:** 25.01.0

- **DPDK**
  - **dpdk-24.11-cn10k**
  - **Version:** 25.01.0

- **OVS**
  - **ovs-3.4.1-cn10k**
  - **Version:** 25.01.0

- **NGINX**
  - **nginx-1.22.0-cn10k**
  - **Version:** 25.01.0

- **OpenSSL**
  - **openssl-1.1.1q-cn10k**
  - **Version:** 25.01.0

- **DPDK based OpenSSL Engine**
  - **openssl-engine-1.0.0-cn10k**
  - **Version:** 25.01.0

- **VPP**
  - **vpp-24.02.0-cn10k**
  - **Version:** 25.01.0

- **octep-target**
  - **oct-ep-target-cn10k**
  - **Version:** 25.01.0

- **firmware-cpt**
  - **cpt-firmware-cn10k**
  - **Version:** 24.09.0

- **firmware-ml**
  - **ml-firmware-cn10k**
  - **Version:** 24.09.0

- **ML models**
  - **ml-models-cn10k**
  - **Version:** 25.01.0

- **Snort**
  - **snort-3-cn10k**
  - **Version:** 25.01.0

Removed Items
-------------

API Changes
-----------

ABI Changes
-----------

export LD_PRELOAD="/lib/aarch64-linux-gnu/librte_acl.so /lib/aarch64-linux-gnu/librte_argparse.so /lib/aarch64-linux-gnu/librte_bbdev.so /lib/aarch64-linux-gnu/librte_bitratestats.so /lib/aarch64-linux-gnu/librte_bpf.so /lib/aarch64-linux-gnu/librte_bus_pci.so /lib/aarch64-linux-gnu/librte_bus_vdev.so /lib/aarch64-linux-gnu/librte_cfgfile.so /lib/aarch64-linux-gnu/librte_cmdline.so /lib/aarch64-linux-gnu/librte_common_cnxk.so /lib/aarch64-linux-gnu/librte_compressdev.so /lib/aarch64-linux-gnu/librte_crypto_cnxk.so /lib/aarch64-linux-gnu/librte_cryptodev.so /lib/aarch64-linux-gnu/librte_dispatcher.so /lib/aarch64-linux-gnu/librte_distributor.so /lib/aarch64-linux-gnu/librte_dma_cnxk.so /lib/aarch64-linux-gnu/librte_dmadev.so /lib/aarch64-linux-gnu/librte_eal.so /lib/aarch64-linux-gnu/librte_efd.so /lib/aarch64-linux-gnu/librte_ethdev.so /lib/aarch64-linux-gnu/librte_event_cnxk.so /lib/aarch64-linux-gnu/librte_eventdev.so /lib/aarch64-linux-gnu/librte_fib.so /lib/aarch64-linux-gnu/librte_gpudev.so /lib/aarch64-linux-gnu/librte_graph.so /lib/aarch64-linux-gnu/librte_gro.so /lib/aarch64-linux-gnu/librte_gso.so /lib/aarch64-linux-gnu/librte_hash.so /lib/aarch64-linux-gnu/librte_ip_frag.so /lib/aarch64-linux-gnu/librte_ipsec.so /lib/aarch64-linux-gnu/librte_jobstats.so /lib/aarch64-linux-gnu/librte_kvargs.so /lib/aarch64-linux-gnu/librte_latencystats.so /lib/aarch64-linux-gnu/librte_log.so /lib/aarch64-linux-gnu/librte_lpm.so /lib/aarch64-linux-gnu/librte_mbuf.so /lib/aarch64-linux-gnu/librte_member.so /lib/aarch64-linux-gnu/librte_mempool.so /lib/aarch64-linux-gnu/librte_mempool_cnxk.so /lib/aarch64-linux-gnu/librte_mempool_ring.so /lib/aarch64-linux-gnu/librte_meter.so /lib/aarch64-linux-gnu/librte_metrics.so /lib/aarch64-linux-gnu/librte_ml_cnxk.so /lib/aarch64-linux-gnu/librte_mldev.so /lib/aarch64-linux-gnu/librte_net.so /lib/aarch64-linux-gnu/librte_net_cnxk.so /lib/aarch64-linux-gnu/librte_net_ring.so /lib/aarch64-linux-gnu/librte_net_tap.so /lib/aarch64-linux-gnu/librte_node.so /lib/aarch64-linux-gnu/librte_pcapng.so /lib/aarch64-linux-gnu/librte_pci.so /lib/aarch64-linux-gnu/librte_pdcp.so /lib/aarch64-linux-gnu/librte_pdump.so /lib/aarch64-linux-gnu/librte_pipeline.so /lib/aarch64-linux-gnu/librte_port.so /lib/aarch64-linux-gnu/librte_power.so /lib/aarch64-linux-gnu/librte_rawdev.so /lib/aarch64-linux-gnu/librte_rcu.so /lib/aarch64-linux-gnu/librte_regexdev.so /lib/aarch64-linux-gnu/librte_reorder.so /lib/aarch64-linux-gnu/librte_rib.so /lib/aarch64-linux-gnu/librte_ring.so /lib/aarch64-linux-gnu/librte_sched.so /lib/aarch64-linux-gnu/librte_security.so /lib/aarch64-linux-gnu/librte_stack.so /lib/aarch64-linux-gnu/librte_table.so /lib/aarch64-linux-gnu/librte_telemetry.so /lib/aarch64-linux-gnu/librte_timer.so /lib/aarch64-linux-gnu/librte_vhost.so"
