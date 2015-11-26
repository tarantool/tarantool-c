curl http://tarantool.org/dist/public.key | sudo apt-key add -
echo "deb http://tarantool.org/dist/master/ubuntu/ `lsb_release -c -s` main" | sudo tee -a /etc/apt/sources.list.d/tarantool.list
sudo apt-get update > /dev/null
sudo apt-get -q install tarantool tarantool-dev
sudo apt-get -q install python-yaml

cmake . -DCMAKE_BUILD_TYPE=Debug
make
sudo make test
