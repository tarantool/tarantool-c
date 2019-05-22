#!/bin/bash

set -e

lcov --capture --directory odbc --output-file coverage.info


#lcov --compat-libtool --directory src/ --capture --output-file coverage.info.tmp
#	lcov --compat-libtool --remove coverage.info.tmp 'tests/*' 'third_party/*' '/usr/*' \
#		--output-file coverage.info
#	lcov --list coverage.info
#
#	@if [ -n "$(COVERALLS_TOKEN)" ]; then \
#		echo "Exporting code coverage information to coveralls.io"; \
#		gem install coveralls-lcov; \
#		echo coveralls-lcov --service-name travis-ci --service-job-id $(TRAVIS_JOB_ID) --repo-token [FILTERED] coverage.info; \
#		coveralls-lcov --service-name travis-ci --service-job-id $(TRAVIS_JOB_ID) --repo-token $(COVERALLS_TOKEN) coverage.info; \
#	fi;