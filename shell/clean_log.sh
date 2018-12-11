#!/bin/bash

dir=$(cd $(dirname $0); pwd)
task_time=$(date "+%Y-%m-%d %H:%M:%S") 

function clean_log() {
  path=$1
  echo "$task_time check log $dir"
  files=`ls -tr $path/*.log*`
  for file in $files
  do
    ft=`date +%s -r $file`
    now=`date +%s`
    ft=$(($ft+604800))
    if [ $now -gt $ft ]; then
      echo "$task_time unlink $file"
      unlink $file
    fi  
  done 
}

clean_log $dir/log
