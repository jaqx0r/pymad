#!/usr/bin/env python

"""Setup script for the MAD module distribution."""

import os
import re
import sys

from distutils.core import setup
from distutils.extension import Extension

VERSION_MAJOR = 0
VERSION_MINOR = 9
PYMAD_VERSION = str(VERSION_MAJOR) + '.' + str(VERSION_MINOR)


def get_setup():
  """Read the configuration data from the Setup file."""
  data = {}
  expr = re.compile(r'(\S+)\s*=\s*(.+)')

  if not os.path.isfile('Setup'):
    print("No 'Setup' file. Perhaps you need to run the configure script.")
    sys.exit(1)

  setup_file = open('Setup', 'r')

  for line in setup_file.readlines():
    match = expr.search(line)
    if not match:
      print('Error in setup file:', line)
      sys.exit(1)
    key = match.group(1)
    val = match.group(2)
    data[key] = val

  return data

SETUP_DATA = get_setup()

DEFINES = [('VERSION_MAJOR', VERSION_MAJOR),
           ('VERSION_MINOR', VERSION_MINOR),
           ('VERSION', '"%s"' % PYMAD_VERSION)]

MADMODULE = Extension(
    name='mad',
    sources=['src/madmodule.c', 'src/pymadfile.c', 'src/xing.c'],
    define_macros=DEFINES,
    include_dirs=[SETUP_DATA['mad_include_dir']],
    library_dirs=[SETUP_DATA['mad_lib_dir']],
    libraries=SETUP_DATA['mad_libs'].split())

setup(  # Distribution metadata
    name='pymad',
    version=PYMAD_VERSION,
    description='A wrapper for the MAD libraries.',
    author='Jamie Wilkinson',
    author_email='jaq@spacepants.org',
    url='http://spacepants.org/src/pymad/',
    license='GPL',
    ext_modules=[MADMODULE])
