#!/bin/bash
#
# based off of -- http://www.linux.com/article.pl?sid=05/08/02/1821218
#
#       /etc/rc.d/init.d/ab_rrd_stats_collector
# This shell script takes care of starting and stopping rrd_stats_collector.py process
#
# Author: Matt Newell <newellm@blur.com>
#
# chkconfig: 2345 20 80
# description: Assburner RRD Stats Collector - Unassigns tasks from assigned hosts, so they can be assigned to idle hosts

# Source function library.
. /etc/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network

# Check that networking is up.
[ ${NETWORKING} = "no" ] && exit 0

usage() {
	echo "Used to start and stop Assburner RRD Stats Collector"
	echo "Usage:"
	echo "    service ab_rrd_stats_collector (stop|start|status|restart)"
	echo
	exit 2;
}


start() {
	echo -n $"Starting Assburner RRD Stats Collector"
	/usr/local/bin/rrd_stats_collector.py -daemonize
	RETVAL=$?
	echo
	[ $RETVAL -eq 0 ] && touch /var/lock/subsys/ab_rrd_stats_collector
	return $RETVAL
}

stop() {
	echo -n $"Shutting down Assburner RRD Stats Collector"
	pid=`ps auxw |grep rrd_stats_collector.py |grep -v grep | awk '{ print $2 }'`
	if [ -n "$pid" ]; then
		if checkpid $pid 2>&1; then
			# term first, then kill if not dead
			kill -TERM $pid >/dev/null 2>&1
			if checkpid $pid && sleep 1 && 
				checkpid $pid && sleep 3 &&
				checkpid $pid ; then
			  	kill -KILL $pid >/dev/null 2>&1
			  	usleep 100000
			fi
		fi
		checkpid $pid
		RC=$?
		[ "$RC" -eq 0 ] && failure $"ab_rrd_stats_collector shutdown" || success $"ab_rrd_stats_collector shutdown"
		RC=$((! $RC))	
	fi
    echo
    rm -f /var/lock/subsys/ab_rrd_stats_collector
    return $RC
}

status() {
	# first see if in process list
	pid=`ps auxw |grep rrd_stats_collector.py |grep -v grep  | awk '{ print $2 }'`
	if [ -n "$pid" ]; then
		echo "ab_rrd_stats_collector (pid $pid) is running..."
		return 0
	fi

	# See if /var/lock/subsys/manager exists
	if [ -f /var/lock/subsys/ab_rrd_stats_collector ]; then
		echo "ab_rrd_stats_collector dead but subsys locked"
		return 2
	fi
	echo "ab_rrd_stats_collector is stopped"
	return 3
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    status)
        status $HOST
        ;;
    restart)
        stop
        start
        ;;
    *)
        echo $"Usage: $0 {start|stop|status|restart}"
        exit 1
        ;;
esac
exit $?
