#! /usr/bin/env python3
# pylama:ignore=D203,D102
"""Simple IGMP utility."""


import sys
import socket
import struct

import easycli as cli


__version__ = '0.1.0'


# UNS verbs
VERB_DISCOVER = 1


GRP = '224.0.0.77'
PORT = 5333
TARGET = (GRP, PORT)


def createsocket():
    """Create a udp socket for IGMP multicast."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 32)

    return sock


class Discover(cli.SubCommand):
    """Discover command line interface."""

    __command__ = 'discover'
    __aliases__ = ['d', 's']
    __arguments__ = [
        cli.Argument('hostname')
    ]

    def __call__(self, args):
        sock = createsocket()
        discover = struct.pack('>B', VERB_DISCOVER)
        sock.sendto(discover + args.hostname.encode(), TARGET)


class Read(cli.SubCommand):
    """Read IGMP packets."""

    __command__ = 'read'
    __aliases__ = ['r']
    __arguments__ = [
    ]

    def __call__(self, args):
        sock = createsocket()
        while True:
            data, host = sock.recvfrom(256)
            print(f'{host}: {data.decode()}')


class UNS(cli.Root):
    """UNS root command line handler."""

    __completion__ = True
    __help__ = 'UNS utility.'
    __arguments__ = [
        cli.Argument('-v', '--version', action='store_true'),
        Discover,
        Read,
    ]

    def __call__(self, args):
        if args.version:
            print(__version__)
            return

        self._parser.print_help()


if __name__ == '__main__':
    UNS.quickstart()
