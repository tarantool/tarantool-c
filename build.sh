mkdir -p rpmbuild/SOURCES

git clone -b $1 https://github.com/tarantool/tarantool-c.git
cd tarantool-c
git submodule update --init --recursive
tar cvf `cat tarantool-c.spec | grep Version: |sed -e  's/Version: //'`.tar.gz . --exclude=.git
sudo yum-builddep -y tarantool-c.spec

cp *.tar.gz ../rpmbuild/SOURCES/
rpmbuild -ba tarantool-c.spec
cd ../

# move source rpm
sudo mv /home/rpm/rpmbuild/SRPMS/*.src.rpm result/

# move rpm, devel, debuginfo
sudo mv /home/rpm/rpmbuild/RPMS/x86_64/*.rpm result/
ls -liah result
