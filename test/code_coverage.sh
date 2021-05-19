#!/bin/bash

lcov --capture --directory ../src/utils -c --output-file utils_lcov.info --no-external
lcov --capture --directory ../src/common -c --output-file common_lcov.info --no-external
lcov --capture --directory ../src/extentmanager -c --output-file em_lcov.info --no-external
lcov --capture --directory ../src/extentserver -c --output-file es_lcov.info --no-external
lcov --capture --directory ../src/kvstore -c --output-file kvstore_lcov.info --no-external

shell="lcov"

if test -s utils_lcov.info; then
    shell+=" -a utils_lcov.info"
fi

if test -s common_lcov.info; then
    shell+=" -a common_lcov.info"
fi

if test -s em_lcov.info; then
    shell+=" -a em_lcov.info"
fi

if test -s es_lcov.info; then
    shell+=" -a es_lcov.info"
fi

if test -s kvstore_lcov.info; then
    shell+=" -a kvstore_lcov.info"
fi

shell+=" -o total.info"

$shell

genhtml -o report total.info
