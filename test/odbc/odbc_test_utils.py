import os
import sys
import subprocess

os.putenv("PRIMARY_PORT",str(33000))

# Runs ODBC test (executable file).
def run_test_by_name(test_name):
    path = os.path.dirname(sys.argv[0])
    if not path:
        path = '.'
        os.chdir(path)
    test_subdir = "odbc"
    progname = './' + os.path.join(test_subdir, test_name)

    try:
        output=subprocess.check_output(progname,stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        output=e.output

    print(output)
