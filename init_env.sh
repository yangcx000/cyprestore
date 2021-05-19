#!/bin/bash

#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

[ ! -d "/etc/cyprestore" ] && mkdir /etc/cyprestore
[ ! -d "/var/log/cyprestore" ] && mkdir /var/log/cyprestore
[ ! -d "/var/lib/cyprestore" ] && mkdir /var/lib/cyprestore

echo "export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib64" >> ~/.bashrc
source ~/.bashrc
