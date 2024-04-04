#!/bin/sh -e

NAME="aesdsocket"

case "$1" in
  status)
        do_status
        ;;
  start)
        echo -n "Starting daemon: "$NAME
        $NAME -d
        echo "."
        ;;
  stop)
        echo -n "Stopping daemon: "$NAME
        kill -9 $(pidof $NAME)
        echo "."
        ;;
  restart)
        echo -n "Restarting daemon: "$NAME
        kill -9 $(pidof $NAME)
        $NAME -d
        echo "."
        ;;
  *)
        echo "Usage: "$1" {start|stop|restart}"
        exit 1
esac

exit 0

