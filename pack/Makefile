all:
	make build-rpm

build-rpm:
	mkdir -p out
	cp build.sh out/
	docker run -v $(CURDIR)/out:/home/rpm/result tarantool/build:$(os)$(dist) sh /home/rpm/result/build.sh

export-rpm:
	package_cloud push karcher/test-repo/$(os)/$(dist)/SRPMS/ out/*.src.rpm --skip-errors
	rm out/*.src.rpm
	package_cloud push karcher/test-repo/$(os)/$(dist)/ out/*.rpm --skip-errors
	rm out/*.rpm
clean:
	docker rm `docker ps -aq`
	rm -rf out
