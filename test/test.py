#!/usr/bin/python

import mad, ao

def play(file):
    mf = mad.MadFile(file)
    
    if mf.layer() == mad.LAYER_I:
        print "MPEG Layer I"
    elif mf.layer() == mad.LAYER_II:
        print "MPEG Layer II"
    elif mf.layer() == mad.LAYER_III:
        print "MPEG Layer III"
    else:
        print "unexpected layer value"
        
    if mf.mode() == mad.MODE_SINGLE_CHANNEL:
        print "single channel"
    elif mf.mode() == mad.MODE_DUAL_CHANNEL:
        print "dual channel"
    elif mf.mode() == mad.MODE_JOINT_STEREO:
        print "joint (MS/intensity) stereo"
    elif mf.mode() == mad.MODE_STEREO:
        print "normal L/R stereo"
    else:
        print "unexpected mode value"
            
    if mf.emphasis() == mad.EMPHASIS_NONE:
        print "no emphasis"
    elif mf.emphasis() == mad.EMPHASIS_50_15_US:
        print "50/15us emphasis"
    elif mf.emphasis() == mad.EMPHASIS_CCITT_J_17:
        print "CCITT J.17 emphasis"
    else:
        print "unexpected emphasis value"
        
    print "bitrate %lu bps" % mf.bitrate()
    print "samplerate %d Hz" % mf.samplerate()
    
    dev = ao.AudioDevice('oss', bits=16, rate=mf.samplerate())
    while 1:
        buffy = mf.read()
        if buffy is None:
            break
        dev.play(buffy, len(buffy))

import sys

if __name__ == "__main__":
    if len(sys.argv) == 1:
        print "playing hardcoded tracks"
        play("/home/jaq/share/music0/Unfiled/super_mario_medley.mp3")
        play("/home/jaq/share/music0/Unfiled/Tom Lehrer - The Old Dope Peddler.wav.mp3")
        play("/home/jaq/share/music0/Unfiled/James Bond - Main Theme.mp3")
    else:
        for file in sys.argv[1:]:
            print "playing %s" % file
            play(file)
        
