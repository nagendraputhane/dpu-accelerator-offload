..  SPDX-License-Identifier: Marvell-MIT
    Copyright (c) 2024 Marvell.

.. rst-class:: home-landing

Data Accelerator Offload (DAO) Documentation
============================================

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Sections

   Introduction <intro>
   Guides <guides>
   Resources <resources>
   Community <community_lab/index>

.. grid:: 1
   :gutter: 2

   .. grid-item-card::
      :text-align: center

      **Introduction**
      ^^^

      What DAO is, architecture, and how these docs are organized.

      +++

      .. button-ref:: intro
         :color: secondary
         :expand:
         :click-parent:

         Read the introduction

   .. grid-item-card::
      :text-align: center

      **Developer Guides**
      ^^^

      Includes Getting Started Guide, Platform Guide, Programmer's Guide, How-to Guides and Tools User Guides.

      +++

      .. button-ref:: gsg/index
         :color: secondary
         :expand:

         Getting Started

      .. button-ref:: platform/index
         :color: secondary
         :expand:

         Platform Guide

      .. button-ref:: prog_guide/index
         :color: secondary
         :expand:

      * :doc:`Common Libraries <prog_guide/common>`
      * :doc:`Conntrack library <prog_guide/conntrack_lib>`
      * :doc:`DMA library <prog_guide/dma_lib>`
      * :doc:`Flow library <prog_guide/flow>`
      * :doc:`Liquid crypto library <prog_guide/liquid_crypto_lib>`
      * :doc:`Netlink library <prog_guide/netlink_lib>`
      * :doc:`VFIO helper <prog_guide/vfio_lib>`
      * :doc:`Virtio crypto lib <prog_guide/virtio_crypto_lib>`
      * :doc:`and many more:- Programmer’s Guide Table of Contents <prog_guide/index>`

      .. button-ref:: applications/index
         :color: secondary
         :expand:

      * :doc:`OVS Offload <applications/ovs-offload>`
      * :doc:`DAO Crypto Agent <applications/crypto-agent>`
      * :doc:`VirtIO Crypto <applications/virtio-crypto>`
      * :doc:`TLS Proxy with NGINX <applications/tls-proxy-nginx>`
      * :doc:`VPP <applications/vpp>`
      * :doc:`Machine Learning <applications/machine-learning>`
      * :doc:`SNORT <applications/snort>`
      * :doc:`K8s CNI Offload <applications/k8s-cni-offload>`
      * :doc:`and many more:- Application User Guide Table of Contents <applications/index>`

      .. button-ref:: howtoguides/index
         :color: secondary
         :expand:

         How-to Guides

   .. grid-item-card::
      :text-align: center

      **Resources**
      ^^^

      Contributing, Release notes, and FAQs.

      +++

      .. button-ref:: contributing/index
         :color: secondary
         :expand:

         Contribute to DAO

      .. button-ref:: rel_notes/index
         :color: secondary
         :expand:

         DAO Release Notes

      .. button-ref:: faq/index
         :color: secondary
         :expand:

         FAQ

   .. grid-item-card::
      :text-align: center

      **Community Lab**
      ^^^

      Hands-on labs to try DAO features.

      +++

      .. button-ref:: community_lab/vpp_l3fwd_lab
         :color: secondary
         :expand:

         Running VPP L3 forward application

      .. button-ref:: community_lab/vpp_lab
         :color: secondary
         :expand:

         Running VPP applications

      .. button-ref:: community_lab/tls_lab
         :color: secondary
         :expand:

         Running TLS applications


.. |badge-dao| image:: https://img.shields.io/github/v/release/MarvellEmbeddedProcessors/dao?sort=date&filter=dao*
.. |build-dao| image:: https://github.com/MarvellEmbeddedProcessors/dao/actions/workflows/build-cn10k.yml/badge.svg
   :target: https://github.com/MarvellEmbeddedProcessors/dao/actions/workflows/build-cn10k.yml
.. |badge-dpdk| image:: https://img.shields.io/github/v/release/MarvellEmbeddedProcessors/marvell-dpdk?display_name=release
.. |build-dpdk| image:: https://github.com/MarvellEmbeddedProcessors/marvell-dpdk-test/actions/workflows/build-cn10k.yml/badge.svg
   :target: https://github.com/MarvellEmbeddedProcessors/marvell-dpdk-test/actions/workflows/build-cn10k.yml
.. |badge-ovs| image:: https://img.shields.io/github/v/release/MarvellEmbeddedProcessors/dao?sort=date&filter=ovs*
.. |build-ovs| image:: https://github.com/MarvellEmbeddedProcessors/dao/actions/workflows/build-cn10k-ovs.yml/badge.svg
   :target: https://github.com/MarvellEmbeddedProcessors/dao/actions/workflows/build-cn10k-ovs.yml
.. |badge-vpp| image:: https://img.shields.io/github/v/release/MarvellEmbeddedProcessors/dao?sort=date&filter=vpp*
.. |build-vpp| image:: https://github.com/MarvellEmbeddedProcessors/dao/actions/workflows/build-cn10k-vpp.yml/badge.svg
   :target: https://github.com/MarvellEmbeddedProcessors/dao/actions/workflows/build-cn10k-vpp.yml

.. list-table:: DAO package repositories (Ubuntu 24.04)
   :header-rows: 1
   :widths: 20 40 40

   * - **Repository**
     - **Package badge**
     - **CI status**

   * - marvell-dao
     - |badge-dao|
     - |build-dao|

   * - marvell-dpdk
     - |badge-dpdk|
     - |build-dpdk|

   * - marvell-ovs
     - |badge-ovs|
     - |build-ovs|

   * - marvell-vpp
     - |badge-vpp|
     - |build-vpp|
