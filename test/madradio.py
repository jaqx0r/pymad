#! /usr/bin/env python

import glob
import os.path
import socket
import sys

try:
    from urllib.parse import urlparse
except ImportError:
    from urlparse import urlparse

import ao

for p in glob.glob('build/lib.*'):
    sys.path.insert(0, p)

import mad


def madradio(url):
    scheme, netloc, path, params, query, fragment = urlparse(url)
    try:
        host, port = netloc.split(':')
    except ValueError:
        host, port = netloc, 80
    if not path:
        path = '/'
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, int(port)))
    sock.send('GET %s HTTP/1.0\r\n\r\n' % path)
    reply = sock.recv(1500)
    # print repr(reply)
    file = sock.makefile()
    mf = mad.MadFile(file)
    print(('bitrate %lu bps' % mf.bitrate()))
    print(('samplerate %d Hz' % mf.samplerate()))
    dev = ao.AudioDevice(0, rate=mf.samplerate())
    while True:
        buffy = mf.read()
        if buffy is None:
            break
        dev.play(buffy, len(buffy))

if __name__ == '__main__':
    import sys
    try:
        url = sys.argv[1]
    except IndexError:
        # url = 'http://62.67.195.6:8000' # lounge-radio.com
        # url = 'http://63.241.4.18:8069' # xtcradio.com
        url = 'http://mp2.somafm.com:2040'  # somafm
    madradio(url)
