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
@click.option('-i', '--ip', default='127.0.0.1', help='OPC server IP address')
@click.option('-p', '--port', default=7890, help='OPC server port')
def main(framerate, ip, port):

    client = opc.Client(ip + ':' + str(port))
    x_dim = 60
    y_dim = 32
    k = 0
    frame = np.zeros((y_dim, x_dim, 3), dtype=np.uint8)

    while True:
        start_time = time.time()

        frame.fill(0)
        frame[k] = [[255, 0, 0] for i in range(60)]

        k = (k + 1) % 32

        client.put_pixels(frame.reshape((x_dim*y_dim, -1)), channel=0)

        time.sleep(max(1.0/framerate - (time.time() - start_time), 0))


if __name__ == '__main__':
    main()
