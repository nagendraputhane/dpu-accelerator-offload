.. SPDX-License-Identifier: Marvell-MIT
   Copyright (c) 2025 Marvell.

DAO 25.05.0 Release Notes
=========================

The following document serves as the **authoritative template** for DAO 25.05.0 release notes.
Fill in the placeholders as you prepare content for publication.

.. note::
   DAO (Data Accelerator Offload) provides libraries and reference
   applications that enable developers to build high-performance
   networking, security, and storage solutions on Marvell OCTEON-based
   DPUs and Arm® server SoCs.

Release Overview
----------------

*Describe in one or two sentences what the new release delivers at a high level.*

Highlights
----------

*Summarise the most significant changes that affect users and integrators.*

- **Component additions/removals** – *e.g. new crypto-offload service, deprecated «foo» library removed.*
- **Version bumps**             – *e.g. DPDK 24.11 ➜ 25.03, GCC 13 ➜ 14.*
- **Breaking changes**          – *APIs removed, configuration defaults changed, tooling switched, etc.*
- **Platform support**          – *new SoCs, board revisions, firmware updates.*

New Packages / Features
-----------------------

*List any brand-new libraries, tools, or reference applications.*

Documentation Updates
---------------------

*Call out new or substantially updated guides, white-papers, or tutorials.*

Operating-System & Tool-chain Support
-------------------------------------

*Document minimum/validated distributions, compilers, linkers, and firmware versions.*

Removed Functionality
---------------------

*Enumerate anything dropped from the distribution.*

Component Changes
-----------------

*For each DAO package, library, or application add a subsection using the
form shown below. Remove sections that do not apply.*

Example section
^^^^^^^^^^^^^^^

.. code-block:: rst

   Component ABC
   ~~~~~~~~~~~~~

   - **Version:** 3.2.1
   - **Dependencies:** libc >=2.38, DPDK >=25.03
   - **Source repo / patches:** `git@example.com:mvl/abc.git`, commits 12a3b4..9f0e1
   - **Changes:** Improved Rx burst routine; replaced deprecated ioctl path.
   - **Notes:** Behaviour change in *_abc_init()*—now returns errno on failure.
   - **Notices:** *None.*

.. tip::
   Copy the block above for every component that changed and edit the
   fields. Use present-tense, concise bullet points.

Known Issues
------------

*Optional – list outstanding problems and work-arounds.*

Upgrade Notes
-------------

*Optional – guidance, migration scripts, configuration changes.*

.. rubric:: Additional Information

- `DAO Programmer’s Guide <https://marvellembeddedprocessors.github.io/dao/guides/>`_
- Support: *dao-support@marvell.com*
