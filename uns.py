#! /usr/bin/env python3
# pylama:ignore=D203,D102,D107,E741,D105,E702,D103
"""UNS utility."""

import re
import sys
import socket
import struct
from os import path, environ

import easycli as cli


__version__ = '0.2.0'


# UNS verbs
VERB_DISCOVER = 1
VERB_ANSWER = 2


GRP = '224.0.0.70'
PORT = 5333
TARGET = (GRP, PORT)
DEFAULT_DBFILE = path.join(environ['HOME'], '.local', 'uns')


IPPART_REGEX = r'\d{1,3}'
IP_REGEX = r'.'.join([IPPART_REGEX] * 4)
HOSTS_REGEX = r'[\w\s.]+'


class DB:
    """Simple database for UNS records."""

    linepattern = re.compile(fr'({IP_REGEX})[\s\t]+({HOSTS_REGEX})')
    ignorepattern = re.compile(r'^[\s#]')

    def __init__(self, filename):
        self.filename = filename
        self._db = {}
        self._names = {}

    def append(self, addr, *hosts):
        self._db[addr] = hosts
        for h in hosts:
            self._names[h] = addr

    def parseline(self, l):
        if self.ignorepattern.match(l):
            return

        m = self.linepattern.match(l)
        if not m:
            raise ValueError(f'Cannot parse: {l}')

        addr, hosts = m.groups()
        hosts = hosts.strip().split(' ')
        self.append(addr, *hosts)

    def save(self):
        with open(self.filename, 'w') as f:
            for addr, hosts in self._db.items():
                f.write(f'{addr} {" ".join(hosts)}\n')

    def resolve(self, hostname):
        addr = self._names.get(hostname)
        if not addr:
            raise KeyError('Cannot find: %s', hostname)

        return hostname, addr

    def find(self, pattern):
        for addr, hosts in self._db.items():
            for h in hosts:
                if h.startswith(pattern):
                    yield h, addr

    def __enter__(self):
        if path.exists(self.filename):
            with open(self.filename) as f:
                for l in f:
                    self.parseline(l)

        return self

    def __exit__(self, ex, extype, tb):
        self.save()


def createsocket(timeout=None):
    """Create a udp socket for IGMP multicast."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 32)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    if timeout:
        sock.settimeout(timeout)

    return sock


def readpacket(sock):
    data, host = sock.recvfrom(256)
    return data[0], data[1:].decode(), host[0], host[1]


def printrecord(args, name, addr, cache):
    if args.short:
        print(addr)
    else:
        flags = ' [cache]' if cache else ''
        print(f'{addr} {name}{flags}')


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


class Find(cli.SubCommand):
    """Find an ip address by it's name."""

    __command__ = 'find'
    __aliases__ = ['f']
    __arguments__ = [
        cli.Argument('pattern'),
        cli.Argument('--no-cache', action='store_true'),
        cli.Argument('-s', '--short', action='store_true'),
        cli.Argument('-t', '--timeout', type=int, default=5,
                     help='Seconds wait before exit, 0: infinite, default: 5'),
    ]

    def online(self, db, args):
        sock = createsocket(args.timeout)
        discover = struct.pack('>B', VERB_DISCOVER)
        sock.sendto(discover + args.pattern.encode(), TARGET)
        try:
            while True:
                verb, name, addr, _ = readpacket(sock)
                db.append(addr, name)
                printrecord(args, name, addr, False)
        except socket.timeout:
            if not args.short:
                print(f'Timeout reached: {args.timeout}', file=sys.stderr)
                return
        except KeyboardInterrupt:
            print('Terminated by user.', file=sys.stderr)

    def __call__(self, args):
        with DB(args.dbfile) as db:
            if not args.no_cache:
                for name, addr in db.find(args.pattern):
                    printrecord(args, name, addr, True)

            self.online(db, args)


class Resolve(cli.SubCommand):
    """Resolve an ip address by it's name."""

    __command__ = 'resolve'
    __aliases__ = ['r', 'd']
    __arguments__ = [
        cli.Argument('hostname'),
        cli.Argument('--no-cache', action='store_true'),
        cli.Argument('-s', '--short', action='store_true'),
        cli.Argument('-t', '--timeout', type=int, default=0,
                     help='Wait for response, default: 0'),
    ]

    def online(self, db, args):
        sock = createsocket(args.timeout)
        discover = struct.pack('>B', VERB_DISCOVER)
        if not args.short:
            print(f'Sending {args.hostname} to {GRP}:{PORT}')
        sock.sendto(discover + args.hostname.encode(), TARGET)
        try:
            while True:
                verb, name, addr, _ = readpacket(sock)
                db.append(addr, name)
                if name == args.hostname:
                    printrecord(args, name, addr, False)
                    return
        except socket.timeout:
            if not args.short:
                print(f'Timeout reached: {args.timeout}', file=sys.stderr)
                return
        except KeyboardInterrupt:
            print('Terminated by user.', file=sys.stderr)

    def __call__(self, args):
        with DB(args.dbfile) as db:
            if args.no_cache:
                return self.online(db, args)

            try:
                name, addr = db.resolve(args.hostname)
                printrecord(args, name, addr, True)

            except KeyError:
                return self.online(db, args)


class Sniff(cli.SubCommand):
    """Sniff IGMP packets."""

    __command__ = 'sniff'
    __aliases__ = ['s']
    __arguments__ = [
    ]

    def __call__(self, args):
        sock = createsocket()
        print(f'Listening to {GRP}:{PORT}')
        sock.bind(TARGET)
        mreq = struct.pack("4sl", socket.inet_aton(GRP), socket.INADDR_ANY)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        try:
            while True:
                verb, name, addr, port = readpacket(sock)
                print(f'{addr}:{port} {name}')
        except KeyboardInterrupt:
            print('Terminated by user.', file=sys.stderr)


class UNS(cli.Root):
    """UNS root command line handler."""

    __completion__ = True
    __help__ = 'UNS utility.'
    __arguments__ = [
        cli.Argument('-v', '--version', action='store_true'),
        cli.Argument(
            '--dbfile',
            default=DEFAULT_DBFILE,
            help=f'Database file, default: {DEFAULT_DBFILE}'
        ),
        Resolve,
        Answer,
        Sniff,
        Find,
    ]

    def __call__(self, args):
        if args.version:
            print(__version__)
            return

        self._parser.print_help()


if __name__ == '__main__':
    UNS.quickstart()
