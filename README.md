Epilepsia is a user friendly cape for the [beaglebone black](https://beagleboard.org/black) capable of driving up to 32 strips of [neopixels](https://learn.adafruit.com/adafruit-neopixel-uberguide) (actually, WS2812B LEDs) in parallel.

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

TODO

## Example
 
## TODO

 * Software estimation of power consumption
 * Beaglepocket support
 * Dithering
