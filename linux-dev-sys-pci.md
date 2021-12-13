Notes on [this great article][article]


# Device and `/dev`

A device is detected by a driver. A driver is either
- compiled into the monolitic kernel at `/boot/vmlinuz-<release>`, or
- loaded by the running kernel from `/lib/modules/<release>`

Linux populates `/dev/` with detected devices and assigns a name to them.

Thus if a device is inserted but does not emerge in `/dev`, then either
- the corresponding module is not loaded, or
- the module needs to be told to rescan

A device file has a major device number and a minor device number,
which are listed and separated by a comma in the output of `ls -l`:
- major device number identifies the driver. Thus devices with the same major number use the same driver
- minor device number is only used by the driver to distinguish various devices

Device number allocation is documented [in kernel-docs][device-allocation].

`/dev` also includes pseudo-devices like `/dev/zero` and `/dev/null`.


# PCI Devices and `lspci`

PCI devices are listed via `lspci`. Each line of the output begins with
PCI bus address formatted as
```
bus:slot.function
```
or
```
domain:bus:slot.function
```
if `-D` is specified.

`-n` asks `lspci` to display devices in format
```
domain:bus:slot.function class: vendor_code:device_code
```

PCI/PCIe bridges form a tree topology of the connected devices, which can be
listed via `lspci -tv`. The output may look like:

```
...
           +-0d.0-[04-07]----00.0-[05-07]--+-03.0-[06]----00.0  Realtek Semiconductor Co., Ltd. RTL8111/8168/8411 PCI Express Gigabit Ethernet Controller
           |                               \-07.0-[07]----00.0  Realtek Semiconductor Co., Ltd. RTL8111/8168/8411 PCI Express Gigabit Ethernet Controller
...
```

To interpret the output, `[XX-YY]` indicates that the buses connceted to
a bridge are numbered from `XX` to `YY`, and the devices downstream will share
the same bus number. So here the two Ethernet controllers are numbered as
`06:00.0` and `07:00.0` respectively. And the bridges that they are directly connected to
are numbered as `XX:03.0` and `YY:07.0` where `XX` and `YY` are from range `05` to `07`.

Information about a specific device can be displayed via `lspci -s` by giving the
device number as above.


# USB Devices and `lsusb`

USB devices are listed as

```
Bus 001 Device 004: ID 0424:7800 Standard Microsystems Corp.
```

To see full details do `lsusb -s 001:004`.


# Disks and `lsscsi`

Devices are listed as 
```
[0:0:0:0]    disk    ATA      VBOX HARDDISK    1.0   /dev/sda
[2:0:0:0]    cd/dvd  VBOX     CD-ROM           1.0   /dev/sr0
```

The 4-tuple in the first column is `[scsi_host:channel:target_number:logical_unit_number]`


# `/sys/`

- `/sys/devices` hierarchy of detected devices, this should give a long name that describes how the device is connected to the bus
- `/sys/bus` supported bus types
- `/sys/class` list of device classes
- `/sys/block` symlinks to block detected devices

Find name under `/dev/` for a disk with a specific label, UUID, etc:
```
tree -F /dev/disk
```

Files are attributes of objects, while symlinks are used to represent relationships between objects.
Some attributes are writable to which values could be `echo`-ed in order to tune for performance.


[article]: https://cromwell-intl.com/open-source/sysfs.html
[device-allocation]: https://www.kernel.org/doc/html/latest/admin-guide/devices.html
