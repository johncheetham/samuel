#!/usr/bin/python

#
# start samuel from within the source folder
# you must run 'python setup.py build' to build the engine before this will run
#

import sys
import os
import sysconfig

assert sys.version_info >= (3,0)

build_lib = "lib.%s-%s" % (sysconfig.get_platform(), sysconfig.get_python_version())
pypath = os.path.join("build", build_lib, "samuel")

sys.path.append(pypath)

import samuel.samuel
samuel.samuel.run()
