[![Build Status](https://travis-ci.com/fyhertz/epilepsia.svg?branch=master)](https://travis-ci.com/fyhertz/epilepsia) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/669e6cb97e7744f1b05ce66e085c8596)](https://www.codacy.com/app/fyhertz/epilepsia?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=fyhertz/epilepsia&amp;utm_campaign=Badge_Grade)

## Introduction

Epilepsia is a user friendly cape for the [beaglebone](https://beagleboard.org/black) capable of driving up to 32 strips of [neopixels](https://learn.adafruit.com/adafruit-neopixel-uberguide) (actually, WS2812B LEDs) in parallel.

![](http://guigui.us/epilepsia/gifs/demo2.gif)
![](http://guigui.us/epilepsia/gifs/demo1.gif)

## Features

 * Can drive 64x32 WS2812B LEDs at around 450-500 fps
 * Support the [Open Pixel Control server](http://openpixelcontrol.org/) and websockets for feeding data.
 * Can be configured for 8, 16 or 32 outputs.
 * Gamma correction and brightness settings

## Architecture

The [led_driver](https://github.com/fyhertz/epilepsia/blob/master/arm/leddriver.cpp) handles the communication with the PRUs. Each PRU can drive 16 WS2812 led strips in parallel using two CD54HC4094 serial to parallel shift register. The bits of the frame buffer are reordered by the beablebone's CPU before being copied to the PRUs shared memory. Each PRU just has to sequentially read that memory to fill its shift registers. The single wire protocol of the led strips is implemented by switching the parallel outputs of the CD54HC4094 IC.

The following schematic shows the wiring the CD54HC4094s and the PRUs:

![Schematic](https://raw.githubusercontent.com/fyhertz/epilepsia/master/schematics/schematic.png)

## Build instructions

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

**You can also compile epilepsia with the provided Dockerfile.** From your clone of the project run:
```
docker build -t epilepsia .
docker run -ti -v `pwd`:/opt/epilepsia epilepsia
```

## Example
 
## TODO

 * Software estimation of power consumption
 * Beaglepocket support
 * Dithering
