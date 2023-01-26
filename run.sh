#!/bin/bash
make Debug
if [ "$?" != "0" ]; then
	exit -1
fi

#sudo setcap CAP_NET_RAW+ep .bin/Debug/netwatcher
#if [ "$?" != "0" ]; then
#	exit -1
#fi

sudo .bin/Debug/udjat-watcher -f

	
