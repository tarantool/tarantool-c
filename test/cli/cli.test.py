#!/usr/bin/env python2

import os
import sys
import subprocess

print('TAP version 13')
print('1..1')


#os.putenv("PRIMARY_PORT",str(server.admin.port))
os.putenv("PRIMARY_PORT",str(33000))



progname = './tarantool-tcp'
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
