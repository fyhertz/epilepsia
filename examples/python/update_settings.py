#!/usr/bin/env python
"""
Demo client.
"""

import click
import opc
import time

@click.command()
@click.option('-i', '--ip', default='127.0.0.1', help='OPC server IP address')
@click.option('-p', '--port', default=7890, help='OPC server port')
@click.option('-b','--brightness', default=0.1, help='Led brightness (from 0 to 1.0)')
@click.option('--dithering/--no-dithering', default=True, help='Enable/disable dithering')
def main(ip, port, brightness, dithering):

    client = opc.Client(ip + ':' + str(port))
    client.set_brightness(brightness)
    client.set_dithering(dithering)


if __name__ == '__main__':
    main()
