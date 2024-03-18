#! /bin/sh

# Ref: Coursera Week 4 Linux System Initialization Video

case "$1" in
    start)
        echo "Starting aesdchar driver"
        #Adding A8
        aesdchar_load
        ;;
    stop)
        echo "Stopping aesdchar driver"
        #Adding A8
        aesdchar_unload
        ;;
    *)
        echo "Usage: $0 {start|stop}"
    exit 1
esac

exit 0
