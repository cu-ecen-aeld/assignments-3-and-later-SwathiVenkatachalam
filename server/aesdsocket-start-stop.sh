#! /bin/sh

# Ref: Coursera Week 4 Linux System Initialization Video

case "$1" in
    start)
        echo "Starting aesdsocket server"
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
        ;;
    stop)
        echo "Stopping aesdsocket server"
        start-stop-daemon -K -n aesdsocket
        ;;
    *)
        echo "Usage: $0 {start|stop}"
    exit 1
esac

exit 0
