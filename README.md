pymad - a Python wrapper for the MPEG Audio Decoder library
===========================================================

![ci](https://github.com/jaqx0r/pymad/workflows/CI/badge.svg)
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
Python 2.0). Run `python setup.py build` to build and then as root run
`python setup.py install`.

if you've installed your mad stuff someplace weird you may need to run
the config_unix.py script, passing it a `--prefix` value to create a
`setup.cfg` file with the correct include and link dirs:

```shell
# python config_unix.py --prefix /usr/local
# python setup.py build
# python setup.py install --prefix /usr/local
```

Remember to make sure `/usr/local/python/site-packages/` is in your
Python search path in that example.

Alternately, you can write `setup.cfg` yourself. E.g.:

    [build_ext]
    library_dirs=/opt/mad/lib
    include_dirs=/opt/mad/include
    libraries=name_of_library_mad_might_depend_on
