#!/usr/bin/env python2

import os
import sys
import subprocess

progname = './connection.test'
path = os.path.dirname(sys.argv[0])
if not path:
    path = '.'
    os.chdir(path)

result='ok 1'
output=''
try:
    output=subprocess.check_output(progname,stderr=subprocess.STDOUT)
except subprocess.CalledProcessError as e:
    output=e.output
    result = 'not '+ result

sys.stderr.write(output)
print(result)
