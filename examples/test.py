#!/usr/bin/env python
"""
Demo client that lights each row one after the other.
"""

import click
import opc
import time
import numpy as np



@click.command()
@click.option('-f', '--framerate', default=120.0, help='Frames per second')
@click.argument('server')
def main(server, framerate):

    client = opc.Client(server)

    x_dim = 60
    y_dim = 32
    frame = np.zeros((y_dim, x_dim, 3), dtype=np.uint8)

    k = 0

    while True:

        frame.fill(0)
        frame[k] = [[0, 0, 100] for i in range(60)]

        k = (k + 1) % 32

        client.put_pixels(frame.reshape((x_dim*y_dim, -1)), channel=0)
        time.sleep(1 / framerate)


if __name__ == '__main__':
    main()