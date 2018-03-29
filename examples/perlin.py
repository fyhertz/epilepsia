# !/usr/bin/env python
"""
Demo client.
"""

import click
import opc
import time
import numpy as np
from noise import pnoise3


@click.command()
@click.option('-f', '--framerate', default=120.0, help='Frames per second')
@click.argument('server')
def main(server, framerate):

    x_dim = 60
    y_dim = 32
    speed = 2.0

    client = opc.Client(server)
    frame = np.zeros((y_dim, x_dim, 3), dtype=np.uint8)

    dt = 1.0 / framerate
    t = 0
    freq_x = 16.0
    freq_y = 16.0
    octaves = 1


    def f(x):
        if x*512>255:
            return 255
        if x<0:
            return 0
        return x*512

    while True:

        frame.fill(0)

        for i in range(y_dim):
            for j in range(x_dim):
                rv = int(f(pnoise3(i / freq_y + 00000, j / freq_x + 00000, t, octaves=octaves)))
                gv = int(f(pnoise3(i / freq_y + 10000, j / freq_x + 10000, t, octaves=octaves)))
                bv = int(f(pnoise3(i / freq_y + 20000, j / freq_x + 20000, t, octaves=octaves)))
                frame[i][j] = (rv, gv, bv)

        t = t + speed*dt

        client.put_pixels(frame.reshape((x_dim * y_dim, -1)), channel=0)
        time.sleep(dt)


if __name__ == '__main__':
    main()
