#!/usr/bin/env python

import os
import sys
import shlex
import shutil
import subprocess

import time

from lib.tarantool_server import TarantoolServer

from pprint import pprint

def compile_cmd(name):
    cmd = ''
    name = os.path.abspath(name)
    if '--valgrind' in sys.argv:
        cmd = cmd + 'valgrind --leak-check=full --log-file=valgrind.out '
        cmd = cmd + '--suppressions=shared/valgrind.sup '
        cmd = cmd + '--keep-stacktraces=alloc-and-free --freelist-vol=2000000000 '
        cmd = cmd + '--malloc-fill=0 --free-fill=0 '
        cmd = cmd + '--num-callers=50 ' + name
    elif '--gdb' in sys.argv:
        cmd = cmd + 'gdb ' + name
    elif '--strace' in sys.argv:
        cmd = cmd + 'strace ' + name
    else:
        cmd = cmd + name
    return cmd

def main():
    path = os.path.dirname(sys.argv[0])
    if not path:
        path = '.'
    os.chdir(path)
    retval = True
    try:
        srv = TarantoolServer()
        srv.script = 'shared/box.lua'
        srv.start()
        test_cwd = os.path.dirname(srv.vardir)
        cmd = compile_cmd('tarantool-tcp')
        print('Running ' + repr(cmd))
        proc = subprocess.Popen(cmd, shell=True, cwd=test_cwd)
        if (proc.wait() != 0):
            retval = False
    except Exception as e:
        print e
        pass
    finally:
        pass

    try:
        srv = TarantoolServer(unix = True)
        srv.script = 'shared/box.lua'
        srv.start()
        test_cwd = os.path.dirname(srv.vardir)
        cmd = compile_cmd('tarantool-unix')
        print('Running ' + repr(cmd))
        proc = subprocess.Popen(cmd, shell=True, cwd=test_cwd)
        proc.wait()
        if (proc.wait() != 0):
            retval = False
    except Exception as e:
        print e
        pass
    finally:
        pass

    if (retval):
        print "Everything is OK"
    else:
        print "FAILED"

    return (-1 if not retval else 0)

exit(main())
