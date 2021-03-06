#!/usr/bin/python

from __future__ import print_function

import glob
import os.path
import sys

try:
    from urllib.request import urlopen
except ImportError:
    from urllib import urlopen

print(sys.version_info)
PYTHON_VERSION = "{}.{}".format(sys.version_info.major, sys.version_info.minor)

for p in glob.glob("build/lib.*-" + PYTHON_VERSION):
    print("Inserting build path {}".format(p))
    sys.path.insert(0, p)

print((sys.path))

import mad

print(mad.__file__)


def play(u):
    mf = mad.MadFile(u)

    if mf.layer() == mad.LAYER_I:
        print("MPEG Layer I")
    elif mf.layer() == mad.LAYER_II:
        print("MPEG Layer II")
    elif mf.layer() == mad.LAYER_III:
        print("MPEG Layer III")
    else:
        print("unexpected layer value")

    if mf.mode() == mad.MODE_SINGLE_CHANNEL:
        print("single channel")
    elif mf.mode() == mad.MODE_DUAL_CHANNEL:
        print("dual channel")
    elif mf.mode() == mad.MODE_JOINT_STEREO:
        print("joint (MS/intensity) stereo")
    elif mf.mode() == mad.MODE_STEREO:
        print("normal L/R stereo")
    else:
        print("unexpected mode value")

    if mf.emphasis() == mad.EMPHASIS_NONE:
        print("no emphasis")
    elif mf.emphasis() == mad.EMPHASIS_50_15_US:
        print("50/15us emphasis")
    elif mf.emphasis() == mad.EMPHASIS_CCITT_J_17:
        print("CCITT J.17 emphasis")
    else:
        print("unexpected emphasis value")

    print(("bitrate %lu bps" % mf.bitrate()))
    print(("samplerate %d Hz" % mf.samplerate()))
    sys.stdout.flush()
    #millis = mf.total_time()
    #secs = millis / 1000
    # print "total time %d ms (%dm%2ds)" % (millis, secs / 60, secs % 60)

    #dev = ao.AudioDevice(0, rate=mf.samplerate())
    while 1:
        buffy = mf.read()
        if buffy is None:
            break
        #dev.play(buffy, len(buffy))
        # print "current time: %d ms" % mf.current_time()

if __name__ == "__main__":
    print(("pymad version %s" % mad.__version__))
    for filename in sys.argv[1:]:
        if os.path.exists(filename):
            filename = "file://" + filename
        u = urlopen(filename)
        if u:
            # if os.path.exists(file):
            print(("playing %s" % filename))
            play(u)
