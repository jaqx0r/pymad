#!/usr/bin/env python

"""Setup script for the MAD module distribution."""

from distutils.core import setup
from distutils.extension import Extension
from setuptools_scm import get_version

scm_kwargs = {
    # Must be in sync with pyproject.toml.
    "tag_regex": r"^version/(?P<version>[^\+]+)(?P<suffix>.*)?$",
}

version = get_version(**scm_kwargs)

MADMODULE = Extension(
    name="mad",
    sources=["src/madmodule.c", "src/pymadfile.c", "src/xing.c"],
    define_macros=[
        ("VERSION", '"{}"'.format(version)),
    ],
    libraries=["mad"],
)

setup(
    version=version,
    ext_modules=[MADMODULE],
    use_scm_version=scm_kwargs,
)
