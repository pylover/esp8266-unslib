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
VERB_ANSWER = 2


GRP = '224.0.0.70'
PORT = 5333
TARGET = (GRP, PORT)


def createsocket():
    """Create a udp socket for IGMP multicast."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 32)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    return sock


class Answer(cli.SubCommand):
    """Answer command line interface."""

    __command__ = 'answer'
    __aliases__ = ['a', 'ans']
    __arguments__ = [
        cli.Argument('hostname', default=GRP),
        cli.Argument('-a', '--address', default=GRP, help=f'Default: {GRP}'),
        cli.Argument('-p', '--port', default=PORT, type=int,
                     help=f'Default: {PORT}'),
    ]

    def __call__(self, args):
        answer = args.hostname
        sock = createsocket()
        verb = struct.pack('>B', VERB_ANSWER)
        print(f'Answering {answer} to {args.address}:{args.port}')
        sock.sendto(verb + answer.encode(), (args.address, args.port))


class Discover(cli.SubCommand):
    """Discover command line interface."""

    __command__ = 'discover'
    __aliases__ = ['d', 's']
    __arguments__ = [
        cli.Argument('hostname'),
        cli.Argument('-s', '--short', action='store_true'),
        cli.Argument('-w', '--wait', action='store_true',
                     help='Wait for response'),
    ]

    def print(self, args, data, host):
        if args.short:
            print(host[0])
        else:
            print(f'{":".join(map(str, host))}: {data.decode()}')

    def __call__(self, args):
        sock = createsocket()
        if not args.short:
            print(f'Sending {args.hostname} to {GRP}:{PORT}')
        discover = struct.pack('>B', VERB_DISCOVER)
        sock.sendto(discover + args.hostname.encode(), TARGET)
        if args.wait:
            while True:
                data, host = sock.recvfrom(256)
                self.print(args, data, host)
        else:
            data, host = sock.recvfrom(256)
            self.print(args, data, host)


class Read(cli.SubCommand):
    """Read IGMP packets."""

    __command__ = 'read'
    __aliases__ = ['r']
    __arguments__ = [
    ]

    def __call__(self, args):
        sock = createsocket()
        print(f'Listening to {GRP}:{PORT}')
        sock.bind(TARGET)
        mreq = struct.pack("4sl", socket.inet_aton(GRP), socket.INADDR_ANY)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        while True:
            data, host = sock.recvfrom(256)
            print(f'{host}: {data[1:].decode()}')


class UNS(cli.Root):
    """UNS root command line handler."""

    __completion__ = True
    __help__ = 'UNS utility.'
    __arguments__ = [
        cli.Argument('-v', '--version', action='store_true'),
        Discover,
        Read,
        Answer,
    ]

    def __call__(self, args):
        if args.version:
            print(__version__)
            return

        self._parser.print_help()


if __name__ == '__main__':
    UNS.quickstart()
