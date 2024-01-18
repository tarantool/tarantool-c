#!/usr/bin/env python3

import os
import subprocess

suite_name = 'cli'

test_name = 'tarantool-tcp'
path = os.path.join(os.environ['BUILDDIR'], 'test', suite_name, test_name)

new_env = os.environ.copy()
new_env["LISTEN"] = f"localhost:{server.get_iproto_port()}"

obj = subprocess.Popen([path], stderr = subprocess.STDOUT,
                       stdout = subprocess.PIPE, universal_newlines=True, env=new_env)
rv = obj.communicate()

print("TAP version 13")
print(rv[0])
