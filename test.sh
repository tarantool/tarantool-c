#!/bin/bash

set -e 

curl http://download.tarantool.org/tarantool/1.7/gpgkey | sudo apt-key add -
release=`lsb_release -c -s`

sudo rm -f /etc/apt/sources.list.d/*tarantool*.list
sudo tee /etc/apt/sources.list.d/tarantool_1.7.list <<- EOF
deb http://download.tarantool.org/tarantool/1.7/ubuntu/ $release main
deb-src http://download.tarantool.org/tarantool/1.7/ubuntu/ $release main
EOF

sudo apt-get update
sudo apt-get -y install tarantool tarantool-dev python-yaml

mkdir test-env
virtualenv test-env
source test-env/bin/activate
pip install -r test-run/requirements.txt
cmake . -DCMAKE_BUILD_TYPE=Debug
make
make test
deactivate
rm -rf test-env

