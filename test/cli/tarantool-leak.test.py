#!/usr/bin/env python3

import os
import subprocess

suite_name = 'cli'

test_name = 'tarantool-leak'
path = os.path.join(os.environ['BUILDDIR'], 'test', suite_name, test_name)

obj = subprocess.Popen([path], stderr = subprocess.STDOUT, stdout = subprocess.PIPE, universal_newlines=True)
rv = obj.communicate()

print("TAP version 13")
print(rv[0])
