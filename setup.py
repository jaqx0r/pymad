#!/usr/bin/env python

"""Setup script for the MAD module distribution."""

from distutils.core import setup
from distutils.extension import Extension

from setuptools_scm import get_version

MADMODULE = Extension(
    name="mad",
    sources=["src/madmodule.c", "src/pymadfile.c", "src/xing.c"],
    define_macros=[
        ("VERSION", '"{}"'.format(get_version())),
    ],
    libraries=["mad"],
)

setup(
    ext_modules=[MADMODULE],
)
