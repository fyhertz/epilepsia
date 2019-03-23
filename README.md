[![Build Status](https://travis-ci.com/fyhertz/epilepsia.svg?branch=master)](https://travis-ci.com/fyhertz/epilepsia) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/669e6cb97e7744f1b05ce66e085c8596)](https://www.codacy.com/app/fyhertz/epilepsia?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=fyhertz/epilepsia&amp;utm_campaign=Badge_Grade) [![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

## Introduction

Epilepsia is a user friendly cape for the [beaglebone](https://beagleboard.org/black) capable of driving up to 32 strips of [neopixels](https://learn.adafruit.com/adafruit-neopixel-uberguide) (actually, WS2812B LEDs) in parallel.

![](http://guigui.us/epilepsia/images/img1.jpg)
![](http://guigui.us/epilepsia/images/img2.jpg)
![](http://guigui.us/epilepsia/images/img3.jpg)
![](http://guigui.us/epilepsia/gifs/demo2small.gif)

## Features

 * Can drive 64x32 WS2812B LEDs at around 450-500 fps
 * Support the [Open Pixel Control](http://openpixelcontrol.org/) protocol and websockets for feeding data.
 * Can be configured for 8, 16 or 32 outputs.
 * Gamma correction and brightness settings

## Architecture

The [led_driver](https://github.com/fyhertz/epilepsia/blob/master/arm/leddriver.cpp) class handles the communication with the PRUs. Each PRU can drive 16 WS2812 led strips in parallel using two 74HC4094 serial to parallel shift register. The bits of the frame buffer are reordered by the beablebone's CPU before being copied to the PRUs shared memory. Each PRU just has to sequentially read that memory to fill its shift registers. The single wire protocol of the led strips is implemented by switching the parallel outputs of the 74HC4094 IC.

The following schematic shows the wiring the 74HC4094s and the PRUs:

![Schematic](https://raw.githubusercontent.com/fyhertz/epilepsia/master/schematics/schematic.png)

## Build instructions

**You can compile epilepsia and build a debian package with the provided Dockerfile:** 
```
git clone https://github.com/fyhertz/epilepsia.git
cd epilepsia
docker build -t fyhertz/epilepsia -f docker/Dockerfile docker
docker run -ti -v `pwd -P`:/data fyhertz/epilepsia
```

The [-v option](https://docs.docker.com/storage/volumes/) is needed to mount the project files in the container. On Windows/Mac OS you might want to replace the `pwd` command with the path to the your clone of the project.

**Cross compiling epilepsia from a linux host:**

1. You will need the clpru C/C++03 compiler from TI to build the PRU firmware:
```
wget http://software-dl.ti.com/codegen/esd/cgt_public_sw/PRU/2.2.1/ti_cgt_pru_2.2.1_linux_installer_x86.bin -O clpru.bin
chmod +x clpru.bin
./clpru.bin --mode unattended --prefix /opt/ti
```

2. A C++14 cross compiler is needed for building the ARM linux binary. On debian/ubuntu run:
```
sudo apt-get update && apt-get install -y make g++-arm-linux-gnueabihf
```

3. The project simply uses some plain Makefiles
```
make
```

## Installation instructions

Supported hardware: [beaglebone black](https://beagleboard.org/black), [beaglebone black wireless](https://beagleboard.org/black-wireless), [beaglebone green](https://beagleboard.org/green), [beaglebone green wireless](https://beagleboard.org/green-wireless)

For simplicity sake, this guide assumes that you are using, or are willing to use, a recent firmware image for your device. **As of May 2018**, that means: 
 - **kernel version > 4.9.** Before that, the pru_rproc driver [behaves differently](https://groups.google.com/d/msg/beagleboard/4P9NdglojBo/qqizuGCZAQAJ)
 - **U-boot overlays** which have recently [replaced Kernel overlays](https://groups.google.com/d/msg/beagleboard/P_Y5yjJyuu4/yaZfkXfAAgAJ)
 - **[cape-universal](https://github.com/cdsteinkuehler/beaglebone-universal-io) readily available.** It's a set of device tree overlays that allows you to easily select pin modes with the pin-config command line tool.
 
 If for some reason you don't want to reflash your beaglebone, epilepsia should still work (maybe with some more work) as long as you have the pru_rproc driver (and not the uio_pruss driver).

1. So, if you don't want any headache, get the latest official debian build [here](https://beagleboard.org/latest-images) (with debian 9 as of May 2018) and flash your device with it.
2. On the bb green wireless you need to disable the wireless virtual overlay. Epilepsia requires P8_11 and P9_31. To do that edit /boot/uEnv.txt and **uncomment "disable_uboot_overlay_wireless=1"**. On the bb black, disable hdmi video: **uncomment "disable_uboot_overlay_video=1"**.
3. TODO

## Optional instructions

To avoid unnecessary write access to bb emmc and prolongate its lifespan you can do the following:

1. Use a circular buffer for syslogs: `apt-get install busybox-syslogd; dpkg --purge rsyslog`
2. Use tmpfs for logs and tmp files. Edit /etc/fstab and append:
```
tmpfs           /tmp            tmpfs   nosuid,nodev         0       0
tmpfs           /var/log        tmpfs   nosuid,nodev         0       0
tmpfs           /var/tmp        tmpfs   nosuid,nodev         0       0
```
3. At this point you can remove logrotate `apt remove --purge logrotate`
 
## TODO

 * Software estimation of power consumption
 * Beaglepocket support
 * Frame interpolation
