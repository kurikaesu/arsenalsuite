#!/bin/bash
#
# based off of -- http://www.linux.com/article.pl?sid=05/08/02/1821218
#
#       /etc/rc.d/init.d/ab_joberror_handler
# This shell script takes care of starting and stopping joberror_handler.py process
#
# Author: Matt Newell <newellm@blur.com>
#
# chkconfig: 2345 20 80
# description: Assburner Job Error Handler - Puts hosts to sleep and wakes them up depending on farm load

# Source function library.
. /etc/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network

# Check that networking is up.
[ ${NETWORKING} = "no" ] && exit 0

usage() {
	echo "Used to start and stop Assburner Job Error Handler"
	echo "Usage:"
	echo "    service ab_joberror_handler (stop|start|status|restart)"
	echo
	exit 2;
}


start() {
	echo -n $"Starting Assburner Job Error Handler"
	/usr/local/bin/joberror_handler.py -daemonize
	RETVAL=$?
	echo
	[ $RETVAL -eq 0 ] && touch /var/lock/subsys/ab_joberror_handler
	return $RETVAL
}

stop() {
	echo -n $"Shutting down Assburner Job Error Handler"
	pid=`ps auxw |grep joberror_handler.py |grep -v grep | awk '{ print $2 }'`
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
		[ "$RC" -eq 0 ] && failure $"ab_joberror_handler shutdown" || success $"ab_joberror_handler shutdown"
		RC=$((! $RC))	
	fi
    echo
    rm -f /var/lock/subsys/ab_joberror_handler
    return $RC
}

status() {
	# first see if in process list
	pid=`ps auxw |grep joberror_handler.py |grep -v grep  | awk '{ print $2 }'`
	if [ -n "$pid" ]; then
		echo "ab_joberror_handler (pid $pid) is running..."
		return 0
	fi

	# See if /var/lock/subsys/manager exists
	if [ -f /var/lock/subsys/ab_joberror_handler ]; then
		echo "ab_joberror_handler dead but subsys locked"
		return 2
	fi
	echo "ab_joberror_handler is stopped"
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
