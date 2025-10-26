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

      Programmer’s Guide

      * :doc:`Common <prog_guide/common>`
      * :doc:`Flow library <prog_guide/flow>`
      * :doc:`VFIO helper <prog_guide/vfio_lib>`
      * :doc:`Virtio crypto lib <prog_guide/virtio_crypto_lib>`

      .. button-ref:: applications/index
         :color: secondary
         :expand:

      .. toctree::
         :maxdepth: 2
         :titlesonly:

         applications/index

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

         Contributing

      .. button-ref:: rel_notes/index
         :color: secondary
         :expand:

         Release Notes

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

         VPP L3FWD Lab

      .. button-ref:: community_lab/vpp_lab
         :color: secondary
         :expand:

         VPP Lab

      .. button-ref:: community_lab/tls_lab
         :color: secondary
         :expand:

         TLS Lab
