pymad - a Python wrapper for the MPEG Audio Decoder library
===========================================================

pymad is a Python module that allows Python programs to use the MPEG Audio Decoder library. pymad provides a high-level API, similar to the pyogg module, which makes reading PCM data from MPEG audio streams a piece of cake.

Using pymad is as easy as:

```python
import mad, ao, sys

mf = mad.MadFile(sys.argv[1])
dev = ao.AudioDevice('oss', rate=mf.samplerate())
while 1:
    buf = mf.read()
    if buf is None:
        break
    dev.play(buf, len(buf))
```

pymad uses the Python distutils tool. To build and install pymad, install python and libmad development files, and then:

```shell
# python config_unix.py --prefix /usr/local
# python setup.py build
# python setup.py install --prefix /usr/local
```

Remember to make sure `/usr/local/python/site-packages/` is in your Python search path.


MAD is available at http://www.mars.org/home/rob/proj/mpeg/

Access this module via "import mad" or "from mad import *".  To decode
an mp3 stream, you'll want to create a MadFile object and read data from
that.  You can then write the data to a sound device.  See the example
program in test/ for a simple mp3 player that uses libao for the sound
device.

This wrapper isn't as low level as the C MAD API is, for example, you don't
have to concern yourself with fixed point conversion -- this was done to
make pymad easy to use.
 
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
