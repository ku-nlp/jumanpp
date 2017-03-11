#!/bin/bash

# This script downloads and unpacks a specific version of Eigen library
# http://eigen.tuxfamily.org/

set -e

cd $(dirname $0)

# settings start
version="3.3.3"
dst_folder="libs/eigen3"
# settings end


function wget_or_curl() {
  [ $# -eq 2 ] || { echo "Usage: wget_or_curl <url> <fpath>" && exit 1; }
  if type wget &> /dev/null; then
    local download_cmd="wget -T 10 -t 3 -O"
  else
    local download_cmd="curl -L -o"
  fi
  $download_cmd "$2" "$1"
}

if [ -d ${dst_folder} ]; then
    echo "Folder '${dst_folder}' already exists. Exiting"
else
    echo "Downloading Eigen library"
    fname="$version.tar.bz2"
    if [ ! -f "eigen.$fname" ]; then
      if ! wget_or_curl "http://bitbucket.org/eigen/eigen/get/$fname" eigen.${fname}; then
          echo "Failed to download $fname"
          rm -f eigen.${fname}
          exit 1
      fi
    fi

    tar -xjf eigen.$fname  || exit 1;
    folder_name=$(tar -tjf eigen.$fname | sed 's/^[./]*//' | head -n 1 | cut -d/ -f 1)
    mv -v $folder_name $dst_folder
    rm -f eigen.$fname
fi