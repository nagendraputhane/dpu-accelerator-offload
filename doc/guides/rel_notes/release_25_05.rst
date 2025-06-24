..  SPDX-License-Identifier: Marvell-MIT
    Copyright (c) 2025 Marvell.

DAO 25.05.0 Release Notes
=========================

.. note::
   DAO (Data Accelerator Offload) provides libraries and reference
   applications that enable developers to build high-performance
   networking, security, and storage solutions on Marvell OCTEON-based
   DPUs and Arm server SoCs.

Release Overview
----------------

DAO 25.05.0 introduces Kubernetes CNI offload capabilities, enhances inline IPsec support in VPP, 
and improved modularization of Marvell OpenSSL Engine.

Release Highlights
------------------

- **Component's additions/removals**  –
  - Added 'k8s-cni-offload', a new application to ofload CNI workloads to the DPU.
  - Reorganized OpenSSL Engine internals for maintainability (no user-facing changes).
- **Version bumps**  –
  - DPDK 25.05 -> DPDK 25.07
- **Breaking changes**  –
- **Platform support**  –
- **New Packages / Libraries / Applications**  –
  - 'k8s-cni-offload' (new component for CNI offload)
- **Documentation / Guides**  –
  - New guide for setting up 'k8s-cni-offload' with Cilium.

Operating-System & Toolchain Support
------------------------------------

- **Linux Distributions:**
  - Ubuntu 24.04 LTS 
- **Toolchains:**
  - GCC 14

Deprecated Features
-------------------

Component Changes
-----------------

Example Component
^^^^^^^^^^^^^^^^^

K8s CNI Offload
~~~~~~~~~~~~~~~~

- **Version:** 25.05.0

- **Dependencies:** Cilium ≥ 1.17.0-dev,CPT ≥ 24.09.0,linux kernel ≥ 6.1.67

- **Source repo / patches:**

  - GitHub: https://github.com/MarvellEmbeddedProcessors/k8s-cni-offload

- **Changes:**

  - Initial release of the `k8s-cni-offload` .

- **Notes:**

  - This project introduces a solution to offload Kubernetes networking tasks to Marvell DPUs, leveraging the standardized Container Network Interface (CNI) framework.
  - The initial implementation focuses on offloading Cilium, the most widely adopted CNI, including by hyper-scalers.
  - The architecture is designed to be flexible, enabling future support for offloading other CNIs without requiring changes to Kubernetes itself.
  - A working proof-of-concept (PoC) has been successfully developed with Cilium as the offloaded CNI.
  - Comprehensive documentation for setup, usage, and integration is included in the repository.

- **Notices:**

  - To build and run the project, follow the instructions in the repository's README file.

VPP
~~~
- **Version:** 25.05.0

- **Dependencies:** DPDK ≥ 25.03.0, CPT ≥ 24.09.0

- **Source repo / patches:**

  - GitHub: https://github.com/MarvellEmbeddedProcessors/vpp

- **Changes:**

  - Inline IPsec offload support for OCTEON-10.
  - Inline IPsec inner packet reassembly support for OCTEON-10.

- **Notes:**

  - Disable DPDK plugin in startup.conf while running OCTEON device plugin.
  - Inline IPsec reassembly supports only single-segment fragments.

- **Notices:**


Marvell OpenSSL Engine
~~~~~~~~~~~~~~~~~~~~~~

- **Version:** 25.05.0

- **Dependencies:** DPDK ≥ 25.03.0, CPT ≥ 24.09.0

- **Source repo / patches:**

  - GitHub: https://github.com/MarvellEmbeddedProcessors/marvell-openssl-engine

- **Changes:**

  - Repurposed code for better code organization. However, no changes from user
    API perspective

- **Notes:**

- **Notices:**


Known Issues
------------

.. rubric:: Additional Information

- `DAO Programmer’s Guide <https://marvellembeddedprocessors.github.io/dao/guides/>`_
