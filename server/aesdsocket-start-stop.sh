#! /bin/sh

# Ref: Coursera Week 4 Linux System Initialization Video

case "$1" in
    start)
        echo "Starting aesdsocket server"
        #Adding A8
        /usr/bin/aesdchar_load
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
        ;;
    stop)
        echo "Stopping aesdsocket server"
        #Adding A8
        /usr/bin/aesdchar_unload
        start-stop-daemon -K -n aesdsocket
        ;;
    *)
        echo "Usage: $0 {start|stop}"
    exit 1
esac

exit 0
