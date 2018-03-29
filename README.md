## What is this thing?

Epilepsia is a fat two square meters LED display built with [Neopixels](https://learn.adafruit.com/adafruit-neopixel-uberguide) (well, WS2812B leds), a [beaglebone black](https://beagleboard.org/black) and a few other components. It has a resolution of 60x32 pixels and can run at a maximum of 250 fps.

## Architecture

### Driving the WS2812 led strips

The led panel has 32 rows of 60 pixels but is composed of 16 led strips of 120 pixels. For instance row 2 is chained with row 1 and is upside down.

The [LedDisplay](https://github.com/fyhertz/epilepsia/blob/master/arm/leddisplay.cpp) class exposes a frame buffer that will ultimately be drawn on the led panel. Each PRU drives 8 of the 16 WS2812 led strips (half of the panel) in parallel using a CD54HC4094 serial to parallel shift register. The bits of the buffer are reordered by the beablebone's CPU so that one sequential byte written on a PRU memory corresponds to 1 bit for each strip. Consequently, each PRU can just sequentially access half of the buffer copied to its memory to fill the shift register of the CD54HC4094 IC. The single wire protocol of the led strips is then implemented by switching the parallel outputs of the CD54HC4094 IC.

The following schematic shows the wiring between the strips, the CD54HC4094s and the PRUs:

![Schematic](https://raw.githubusercontent.com/fyhertz/epilepsia/master/doc/schematic.png)

### Supplying power

According to the [adafruit uberguide](https://learn.adafruit.com/adafruit-neopixel-uberguide), at full brightness a pixel can consume as much as 60mA. Therefore, the panel could theoretically consume 600W or 115A @ 5V! 

Based upon my tests 80W (16A @ 5V) is already super bright though. So [one 18 AWG](https://www.powerstream.com/Wire_Size.htm) power cable should be enough to power the panel. 

Anyway when building the thing I chose to wire 32 [cheap LM2596 boards](https://www.youtube.com/watch?v=R32zDhGIGyw), one per row, to get between 1 or 2A per row (between 180W and 320W for the display) without too much current in power cables. That was probably overkill and even inefficient considering 80W is already too bright and that those power modules are quite limited. They only have an efficiency of 80% when converting 12V to 5V which is mediocre and they are not very stable.

I have not actually tried to push the panel at more than a hundred watts yet.

### Feeding the panel

The arm firmware starts an [Open Pixel Control server](http://openpixelcontrol.org/) and waits for client to feed frame buffers. The **examples** folder contains some demo clients for the panel.

## Build instructions

TODO

## Notes

 * The code can probably be modified to handle 4*8 led strips, the PRUs are spending almost half the time idling.
 
 ## TODO
 
 * Software estimation of power consumption.
