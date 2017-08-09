#!/usr/bin/env python

"""Setup script for the MAD module distribution."""

import os
import re
import sys

from distutils.core import setup
from distutils.extension import Extension

VERSION_MAJOR = 0
VERSION_MINOR = 10
PYMAD_VERSION = str(VERSION_MAJOR) + '.' + str(VERSION_MINOR)


DEFINES = [('VERSION_MAJOR', VERSION_MAJOR),
           ('VERSION_MINOR', VERSION_MINOR),
           ('VERSION', '"%s"' % PYMAD_VERSION)]

MADMODULE = Extension(
    name='mad',
    sources=['src/madmodule.c', 'src/pymadfile.c', 'src/xing.c'],
    define_macros=DEFINES,
    libraries=['mad'])

setup(  # Distribution metadata
    name='pymad',
    version=PYMAD_VERSION,
    description='A wrapper for the MAD libraries.',
    author='Jamie Wilkinson',
    author_email='jaq@spacepants.org',
    url='http://spacepants.org/src/pymad/',
    license='GPL',
    ext_modules=[MADMODULE])
