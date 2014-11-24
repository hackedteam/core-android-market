#!/bin/bash -
#===============================================================================
#
#          FILE: updateServer.sh
#
#         USAGE: ./updateServer.sh
#
#   DESCRIPTION:
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: zad (), e.placidi@hackingteam.com
#  ORGANIZATION: ht
#       CREATED: 29/10/2014 11:47:58 CET
#      REVISION:  ---
#===============================================================================
if [ $# -lt 2 ]
then
	echo "usage: $0 serverAddress action[full|restart]"
	exit 0
fi

if [ "$2" == "full" ]
then
	scp -r ./server $1:
	if [ $? -ne 0 ]
	then
		echo "update failed"
		exit 1
	fi
	ssh $1 chmod +x server/benews-srv.py
fi
pid=`ssh $1 ps ax | grep benews-srv.py | grep -v grep| cut -d' ' -f1`
echo "found \"$pid\""
while [ ! -z "$pid" ]
do
	echo "try to kill \"$pid\""
	ssh $1 kill $pid
	sleep 2
	pid=`ssh $1 ps ax | grep benews-srv.py | grep -v grep| cut -d' ' -f1`
done
ssh $1 "cd server && ./benews-srv.py >/dev/null"  &
