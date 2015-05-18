pymad - a Python wrapper for the MPEG Audio Decoder library
===========================================================

[![Build Status](https://travis-ci.org/jaqx0r/pymad.svg?branch=master)](https://travis-ci.org/jaqx0r/pymad)

pymad is a Python module that allows Python programs to use the MPEG Audio Decoder library. pymad provides a high-level API, similar to the pyogg module, which makes reading PCM data from MPEG audio streams a piece of cake.

MAD is available at http://www.mars.org/home/rob/proj/mpeg/

Access this module via `import mad`.  To decode
an mp3 stream, you'll want to create a `mad.MadFile` object and read data from
that.  You can then write the data to a sound device.  See the example
program in `test/` for a simple mp3 player that uses the `python-pyao` wrapper around libao for the sound
device.

pymad wrapper isn't as low level as the C MAD API is, for example, you don't
have to concern yourself with fixed point conversion -- this was done to
make pymad easy to use.

```python
import sys

import ao
import mad

mf = mad.MadFile(sys.argv[1])
dev = ao.AudioDevice(0, rate=mf.samplerate())
while 1:
    buf = mf.read()
    if buf is None:  # eof
        break
    dev.play(buf, len(buf))
```


To build, you need the distutils package, availible from
http://www.python.org/sigs/distutils-sig/download.html (it comes with
Python 2.0). Run "python setup.py build" to build and then as root run
"python setup.py install".  You may need to run the config_unix.py
script, passing it a --prefix value if you've installed your mad stuff
someplace weird.  Alternately, you can just create a file called
"Setup" and put in values for mad_include_dir, mad_lib_dir, and
mad_libs.  The file format for Setup is:

key = value

with one pair per line.

```shell
# python config_unix.py --prefix /usr/local
# python setup.py build
# python setup.py install --prefix /usr/local
```

Remember to make sure `/usr/local/python/site-packages/` is in your Python search path.
