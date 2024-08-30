# Acquire Driver for Hamamatsu DCAM

[![Build](https://github.com/acquire-project/acquire-driver-hdcam/actions/workflows/build.yml/badge.svg)](https://github.com/acquire-project/acquire-driver-hdcam/actions/workflows/build.yml)
[![Tests](https://github.com/acquire-project/acquire-driver-hdcam/actions/workflows/test_pr.yml/badge.svg)](https://github.com/acquire-project/acquire-driver-hdcam/actions/workflows/test_pr.yml)

This is an Acquire Driver that exposes Hamamatsu cameras via the [DCAM-SDK][].

## Devices

### Camera

- **Hamamatsu CP15440-20UP** Orca Fusion BT


## Experimental Linux Support

Early in 2024, Hammamatsu released the [DCAM-API Lite for Linux](https://www.hamamatsu.com/us/en/product/cameras/software/driver-software/dcam-api-lite-for-linux.html).  This contains only the dynamic libraries needed at runtime.  To build this module, one must still download the SDK (above) for the headers... but the (windows) libraries will be ignored. 

[DCAM-SDK]: https://dcam-api.com/sdk-download/
