#!/usr/bin/env python2

from __future__ import print_function
import sys

import os
import subprocess

os.putenv("PRIMARY_PORT", str(33000))

suite_name = 'cli'
test_name  = 'tarantool-poll'

path = os.path.join(os.environ['BUILDDIR'], suite_name, test_name)

obj = subprocess.Popen([path], stderr = subprocess.STDOUT, stdout = subprocess.PIPE)
rv = obj.communicate()

print("TAP version 13")
print(rv[0])

if obj.returncode != 0:
    print("Process ends up with status code %s" % obj.returncode, file=sys.stderr)
    print(rv[0], file=sys.stderr)
    print(rv[1], file=sys.stderr)
