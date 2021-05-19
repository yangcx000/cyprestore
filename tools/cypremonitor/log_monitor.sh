#!/bin/bash
#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

IFS=$'\n'

if [ $# -lt 2 ]; then
	echo "Usage: $0 {app_name} {log_dir}"
	exit 1
fi

cd /export/cypremonitor

app_name=$1
log_dir=$2
ip_address=`ip a | grep bond0 | grep inet | awk '{print $2}' | awk -F / '{print $1}'`
date_time=`date "+%m%d %H:%M" --date '-1 min'`
target_msg_prefix="E${date_time}"

for file in `ls ${log_dir} | grep ${app_name} | grep -v grep`; do
	filepath="${log_dir}/${file}"
	if [ ! -e ${filepath} -o ! -f ${filepath} ]; then
        continue
    fi
		
	module_name=`echo ${file} | awk -F . '{print $1}'`
	for msg in `grep ${target_msg_prefix} ${filepath} | awk -F ] '{print $2}'`; do
		emsg=`echo ${msg} | awk -F , '{print $1}'`	
		rule_key=${module_name}${target_msg_prefix}${emsg}
		text="time:"`date "+%Y-%m-%d %H:%M"`
		text+=", ip:${ip_address}"
		text+=", module:${module_name}"
		text+=", message:${msg}"
		./notify -alarmappname ${app_name} -messagelevel NOTIFICATION -rulekey ${rule_key} -subject ${app_name}"-log-error" -text ${text}
	done
done
