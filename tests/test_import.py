import os
import sys

sys.path.insert(0, os.path.abspath(".."))
print(sys.path)

from importlib.metadata import version


def test_import():
    import mad

    print("Package version:", mad.__version__)
    print("Expected metadata version:", version("pymad"))

    assert mad.__version__ == version("pymad")
