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
        cmd = cmd + 'valgrind --leak-check=full --log-file=valgrind-%s.out ' % os.path.basename(name)
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

def changedir():
    path = os.path.dirname(sys.argv[0])
    if not path:
        path = '.'
    os.chdir(path)

def run_test(name, unix = False):
    retval = True

    try:
        srv = TarantoolServer(unix = unix)
        srv.script = 'shared/box.lua'
        srv.start()
        test_cwd = os.path.dirname(srv.vardir)
        cmd = compile_cmd(name)
        print('Running ' + repr(cmd))
        proc = subprocess.Popen(cmd, shell=True, cwd=test_cwd)
        if (proc.wait() != 0):
            retval = False
    except Exception as e:
        print e
        pass
    finally:
        pass

    return retval

def main():
    changedir()

    retval = True
    retval = retval & run_test('tarantool-tcp', False)
    retval = retval & run_test('tarantool-unix', True)

    if (retval):
        print "Everything is OK"
    else:
        print "FAILED"

    return (-1 if not retval else 0)

exit(main())
