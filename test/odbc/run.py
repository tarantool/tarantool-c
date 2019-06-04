import os
import sys
import subprocess
import re

# odbc/foo.test.py -> odbc/foo.test
test_name_py = test_run_current_test.name
test_name = re.sub(r'^(.*).py$', r'\1', test_name_py)

# create odbcinst.ini
odbc_so_file = os.path.realpath('../odbc/libtnt_odbc.so')
odbc_inst_file = os.path.realpath(os.path.join(server.vardir, 'odbcinst.ini'))
odbc_inst_content = """
[Tarantool]
Description     = Tarantool
Driver          = {}
""".format(odbc_so_file).lstrip()
with open(odbc_inst_file, 'w') as f:
    f.write(odbc_inst_content)

# UnixODBC determines path to odbcinst.ini file by concatenating
# ODBCSYSINI + ODBCINSTINI.
env = {
    'ODBCINSTINI': '{}'.format(odbc_inst_file),
    'ODBCSYSINI': '',
    'LISTEN': server.iproto.uri,
}

test_path = os.path.join(server.vardir, '../..', test_name)
process = subprocess.Popen(test_path, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, env=env, cwd=server.vardir)
stdout, _ = process.communicate()

print('TAP version 13')
if process.returncode == 0:
    print(stdout)
else:
    print(stdout)
    print('[Exited with the code {}]'.format(process.returncode))
