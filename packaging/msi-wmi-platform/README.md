# msi-wmi-platform (DKMS copy)

`msi-wmi-platform.c` is the Linux kernel's MSI WMI platform driver by Armin Wolf (GPL-2.0-or-later), taken from upstream
`drivers/platform/x86/msi-wmi-platform.c`. Kernels from about 6.17 ship it, but many distributions do not enable it
(`CONFIG_MSI_WMI_PLATFORM`). Mcenter builds this copy with DKMS only when the running kernel has no such module.

Local change: `devm_kzalloc_obj(dev, *data)` was replaced by `devm_kzalloc(dev, sizeof(*data), GFP_KERNEL)` so it builds on kernels
that predate that helper. The driver is only loaded on demand by `mcenter-battery` (during a battery calibration) and unloaded again.
