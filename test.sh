#!/bin/bash

set -e

REPO=${TARANTOOL_REPO:-1.7}

curl http://download.tarantool.org/tarantool/${REPO}/gpgkey | sudo apt-key add -
release=`lsb_release -c -s`

sudo rm -f /etc/apt/sources.list.d/*tarantool*.list
sudo tee /etc/apt/sources.list.d/tarantool_${REPO}.list <<- EOF
deb http://download.tarantool.org/tarantool/${REPO}/ubuntu/ $release main
deb-src http://download.tarantool.org/tarantool/${REPO}/ubuntu/ $release main
EOF

sudo apt-get update
sudo apt-get -y install tarantool tarantool-dev python-yaml

mkdir test-env
virtualenv test-env
source test-env/bin/activate
pip install -r test-run/requirements.txt
cmake . -DCMAKE_BUILD_TYPE=Debug
make

ulimit -n 8192 # make fd limit more for 1024-poll test
make test || (find test -name '*.reject' | xargs head -n -1 && exit 1)

deactivate
rm -rf test-env
