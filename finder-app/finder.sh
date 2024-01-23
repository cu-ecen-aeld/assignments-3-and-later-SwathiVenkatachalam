#!/bin/sh
# Filename: finder.sh
# Author: Swathi Venkatachalam
# References: https://ryanstutorials.net/bash-scripting-tutorial/bash-variables.php
#           : https://ryanstutorials.net/bash-scripting-tutorial/bash-if-statements.php

# Accepts the following runtime arguments ($1-$9: First 9 arguments to bash script)
filesdir=$1 #1st argument: path to a directory on the filesystem
searchstr=$2 #2nd argument: text string which will be searched within these files

# Check if parameters above are not specified and return error if not present
if [ "$#" -ne 2 ]
then
	echo "Error: Two parameters <filesdir> and <searchstr> required; Action Required: Enter the 2 arguments mentioned above."
	exit 1
fi

# Check if filesdir represents a directory on the filesystem and return error if not
if [ ! -d "$filesdir" ] # -d checks if file next to it exists and is a directory
then
	echo "Error: <filesdir> argument doesn't represent directory on filesystem; Action Required: Enter valid path to directory on filesystem."
	exit 1
fi

# Number of files in the directory and all subdirectories
X=$(find "$filesdir" -type f | wc -l) #finds files in dir and counts lines of find command
# find "$filesdir" = searches for files in dir
# -type f = option of find that says to search for files only
# | = pipe; takes left command ouput and gives to right command input
# wc -l = wc stands for word count, -l number of lines

# Number of matching lines (line which contains searchstr and may also contain additional content) found in respective files
Y=$(grep -r "$searchstr" "$filesdir" | wc -l) #searches recursively serachstr in filesdir and counts lines of output
# grep = search text pattern in file
# -r = search recursively in dirs
# "$searcstr" = string to search for by grep
# "$filesdir" = path to dir for grep to start search in 
# | = pipe; takes left command ouput and gives to right command input
# wc -l = wc stands for word count, -l number of lines

# Prints message
echo "The number of files are $X and the number of matching lines are $Y"
