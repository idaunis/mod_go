#!/bin/bash

if ! command -v go &> /dev/null ; then
	echo -e 'error: mod_go requires the Go compiler.'
	echo -e '##########################################################'
	echo -e 'To download Go and install from source, run the following:'
	echo -e ' $ hg clone -u release https://code.google.com/p/go'
	echo -e	' $ cd go/src'
	echo -e ' $ ./all.bash'
	echo -e '##########################################################'
	exit -1
fi

if ! command -v gd &> /dev/null ; then
	echo -e 'error: mod_go requires GODAG.'
	echo -e '############################################################'
	echo -e 'To download Godag and install from source run the following:'
	echo -e ' $ hg clone https://godag.googlecode.com/hg/ godag'
	echo -e ' $ go run mk.go install'
	echo -e '############################################################'
	exit -1 
fi  

sudo mkdir -p -m 777 /etc/modgo/.temp /etc/modgo/.bin
gddir=`command -v gd`
sudo ln -s $gddir /etc/modgo/gd &>/dev/null 2>&1
exit 0
