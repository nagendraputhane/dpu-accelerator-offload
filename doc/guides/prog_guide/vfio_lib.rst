..  SPDX-License-Identifier: Marvell-MIT
    Copyright (c) 2024 Marvell.

************
VFIO Library
************
Platform devices in Linux refer to System-on-Chip (SoC) components that aren't situated on standard
buses such as PCI or USB. You can see them in Linux at the path /sys/bus/platform/devices/. To
interact with platform devices from user space, the vfio-platform driver provides a framework. This
library provides DAO APIs built upon this framework, enabling access to the device resources.

Also this library can be used to access the standard PCIe devices present at /sys/bus/pci/devices/
from the user space.

Prerequisites for Platform Devices:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
To make use of VFIO platform framework, the ``vfio-platform`` module must be loaded first:

.. code-block:: console

   sudo modprobe vfio-platform

.. note::

   By default ``vfio-platform`` assumes that platform device has dedicated reset driver. If such
   driver is missing or device does not require one, this option can be turned off by setting
   ``reset_required=0`` module parameter.

Afterwards, the platform device needs to be bound to vfio-platform, following a standard two-step
procedure. Initially, the driver_override, located within the platform device directory, must be
configured to vfio-platform:

.. code-block:: console

   echo vfio-platform | sudo tee /sys/bus/platform/devices/DEV/driver_override

Next ``DEV`` device must be bound to ``vfio-platform`` driver:

.. code-block:: console

   echo DEV | sudo tee /sys/bus/platform/drivers/vfio-platform/bind

Prerequisites for PCIe Devices:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
To make use of VFIO PCIe framework, the ``vfio-pci`` module must be loaded first:

.. code-block:: console

   sudo modprobe vfio-pci

The PCIe device needs to be bound to vfio-pci, following a standard two-step procedure. Initially,
the driver_override, located within the pci device directory, must be configured to vfio-pci:

.. code-block:: console

   echo vfio-pci | sudo tee /sys/bus/pci/devices/<BDF>/driver_override

Next ``BDF`` of the device must be bound to ``vfio-pci`` driver:

.. code-block:: console

   echo <BDF> | sudo tee /sys/bus/pci/drivers/vfio-pci/bind

DAO VFIO device initialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Invoking the `dao_vfio_init()` API creates a VFIO container by opening the /dev/vfio/vfio character
device and initializes the memory used for storing the details of the devices. This API should be
invoked only once to initiate the library.

.. code-block:: c

   int dao_vfio_init(void);

After initializing the library, the `dao_vfio_device_setup()` API can be used to initialize the
device. The function takes the memory for storing the device details, specified by the
`struct dao_vfio_device` argument. Upon successful execution, the resources of the devices are
mapped, and the device structure is populated.

.. code-block:: c

   int dao_vfio_device_setup(const char *dev_name, struct dao_vfio_device *pdev);

.. literalinclude:: ../../../lib/vfio/dao_vfio.h
   :language: c
   :start-at: struct dao_vfio_mem_resouce
   :end-before: End of structure dao_vfio_device.

DAO VFIO device cleanup
~~~~~~~~~~~~~~~~~~~~~~~

`dao_vfio_device_free()` releases the VFIO device and frees the associated memory.

.. code-block:: c

   void dao_vfio_device_free(struct dao_vfio_device *pdev);


Upon closing all open devices, the container can be shut down by calling `dao_vfio_fini()`.

.. code-block:: c

   void dao_vfio_fini(void);

