## What is this thing?

Epilepsia is a cape for the [beaglebone black](https://beagleboard.org/black) capable of driving up to 32 strips of [Neopixels](https://learn.adafruit.com/adafruit-neopixel-uberguide) (actually, WS2812B leds) in parallel.

### Features

 * Can drive 64x32 neopixels at around 450-500 fps
 * Support the [Open Pixel Control server](http://openpixelcontrol.org/) and websockets for feeding data.
 * Can be configured for 8, 16 or 32 outputs. You only have to solder what you need.
 * Gamma correction
 * Brightness parameter

### Architecture

The [led_driver](https://github.com/fyhertz/epilepsia/blob/master/arm/leddriver.cpp) handles the communication with the PRUs. Each PRU can drive 16 WS2812 led strips in parallel using two CD54HC4094 serial to parallel shift register. The bits of the frame buffer are reordered by the beablebone's CPU before being copied to the PRUs shared memory. Each PRU just has to sequentially read that memory to fill its shift registers. The single wire protocol of the led strips is implemented by switching the parallel outputs of the CD54HC4094 IC.

The following schematic shows the wiring the CD54HC4094s and the PRUs:

![Schematic](https://raw.githubusercontent.com/fyhertz/epilepsia/master/schematics/schematic.png)

## Build instructions

TODO

## Example: massive led panel with 60x32 pixels

The two gifs below show the epilepsia cape driving a two square meter led panel:

![](http://guigui.us/epilepsia/gifs/demo2.gif)
![](http://guigui.us/epilepsia/gifs/demo1.gif)

### Prototype board

![](http://guigui.us/epilepsia/images/prototype.jpg)

### Power supply

According to the [adafruit uberguide](https://learn.adafruit.com/adafruit-neopixel-uberguide), at full brightness a pixel can consume as much as 60 mA. Therefore, the panel could theoretically consume 600 W or 115 A @ 5 V! 

Based upon my tests 80 W (16 A @ 5 V) is already super bright though. So [one 18 AWG](https://www.powerstream.com/Wire_Size.htm) power cable should be enough to power the panel. 

On my setup, the strips are wired to two 18 AWG cables which should be enough to distribute 160 W (32 A). They are attached on each side of the panel, one for the even rows and the other for the odd ones. This is to ensure an even weight distribution as well as a more even light distribution (given that the [voltage drops accross each strip](https://learn.adafruit.com/adafruit-neopixel-uberguide/powering-neopixels#distributing-power)).

### Feeding the panel

The epilepsia firmware starts an [Open Pixel Control server](http://openpixelcontrol.org/) server and waits for clients to feed frame buffers. The examples folder contains some demo clients for the panel.
 
## TODO

 * Software estimation of power consumption
 * Beaglepocket support
 * Dithering
