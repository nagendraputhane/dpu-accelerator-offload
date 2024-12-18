..  SPDX-License-Identifier: Marvell-MIT
    Copyright (c) 2024 Marvell.

PCIe EP configuration
#####################

PCIe EP and SDP configurations can be modified during firmware bootup via the EBF menu.
This menu allows runtime changes to the following settings:

* PCIe mode, link selection
* Enabling access to octeon physical memory to host
* Total number of PFs (only for PEM0)
* Total number of VFs per PF (only for PEM0)
* Number of SDP PF rings
* Number of per PF SDP VF rings

While booting, press ``B`` on the serial confolse to enter boot menu. Select ``S`` to enter
setup menu, select ``P`` for PCIe configuration

.. code-block:: console

   ...
   Board Serial:   WA-CN106-A1-PCIE-2P100-R2-147
   Chip:  0xb9 Pass B0
   SKU:   MV-CN10624-B0-AAP
   LLC:   49152 KB
   Boot:  SPI0_CS0,SPI1_CS0, using SPI0_CS0
   AVS:   Enabled

   Press 'B' within 10 seconds for boot menu

   =================================
   Boot Options
   =================================
   1) Boot from Primary Boot Device
   2) Boot from Secondary Boot Device
   N) Boot Normally
   S) Enter Setup
   D) Enter DRAM Diagnostics
   K) Burn boot flash using Kermit
   U) Change baud rate and flow control
   R) Reboot

   Choice: S

   =================================
   Setup
   =================================
   B) Board Manufacturing Data
   C) Chip Features
   D) DRAM Options
   P) PCIe Configuration
   W) Power Options
   E) Ethernet configuration
   F) Restore factory defaults
   G) Misc options
   S) Save Settings and Exit
   X) Exit Setup, discarding changes

   Choice: P

   =================================
   PCIe Port Setup
   =================================
   1) PCIe Mode Selection
   2) PCIe Link Speed Selection
   3) PCIe GEN3 Preset Vector Selection
   4) PCIe GEN4 Preset Vector Selection
   5) PCIe GEN5 Preset Vector Selection
   6) PCIe GEN3 Initial Preset Value Selection
   7) PCIe GEN4 Initial Preset Value Selection
   8) PCIe GEN5 Initial Preset Value Selection
   9) PCIe Reference Clock Selection
   A) PCIe GEN2 DE-Emphasis Selection
   B) PCIe EP Script File Info
   C) PCIe EP ROM Flash File Info
   D) PCIe EP ROM Flash Write Update
   E) PCIe EP ROM Flash Write Verify
   F) PCIe EP ROM Flash Select Default Script
   G) PCIe EP Security
   H) PCIe EP Identity
   I) PCIe Gen2 Low Swing Mode Selection
   L) SDP ring configuration
   Q) Return to main menu

Select ``1`` to enter PCIe mode selection menu w

PCIe EP Security
----------------

Select option ``G`` to enable access to Octeon memory from the host.
Configure the host stream IDs in the IOBN registers on the Octeon side.

.. code-block:: console

   Choice: G

   =================================
   PCIe EP Security
   =================================
   A) Allow PCIe Host to Access Octeon Memory (1)
   S) Host Stream IDs for PEM0 ( ...)
   C) Disable host BAR0 memory window register access (0)
   Q) Return to main menu

Option ``A`` - Default is deny (0). Set this to 1 to allow stream ids configured here.

Option ``S`` allows user to enter the streamid per PEM. Depending on how many PEMs are in EP mode
those many options will be shown.

Select the appropriate PEM option and do as follows

.. code-block:: console

   Choice: S

   SMMU Stream IDs for PEM0.  This is a list of strings.  Each string is a
   C-style hexadecimal number.  For information about the fields of a Stream
   ID, see the "SMMU Stream IDs" section of the CN9XXX HRM.
   Default value is 0x30000 (just one item on the list).
   Current value:

   Enter multiple lines for new value. Input ends with a blank line
   INS)Host Stream IDs for PEM0(line 1): 0x3ffff

Example: for PEM0 to, enter 0x3ffff as stream id to allow all streams.

Select ``Q`` to return to previous menu.

Host side PCI EP configs
------------------------

Select option ``H`` to enter PCI configuration menu, which provide options to change
no of PFs, no of VFs per PF, no of SDP PF rings, no of per PF SDP VF rings, update deviceID,
class code per PF:

.. code-block:: console

   Choice: H

   =================================
   PCIe EP Identity
   =================================
   P) Number of PFs
   V) Number of VFs per PF
   D) DeviceID per PF
   C) Class code per PF
   Q) Return to main menu

Once all changes are done, select ``Q`` to return to the main setup menu and select ``S``
to save the settings and exit.
