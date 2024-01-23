#!/bin/sh
# Filename: writer.sh
# Author: Swathi Venkatachalam
# References: https://ryanstutorials.net/bash-scripting-tutorial/bash-variables.php
#           : https://ryanstutorials.net/bash-scripting-tutorial/bash-if-statements.php

# Check if parameters above are not specified and return error if not present
if [ "$#" -ne 2 ]
then
        echo "Error: Two parameters <writefile> and <writestr> required; Action Required: Enter the 2 arguments mentioned above."
        exit 1
fi

# Accepts the following runtime arguments ($1-$9: First 9 arguments to bash script)
writefile=$1 #1st argument:  full path to a file (including filename) on the filesystem
writestr=$2 #2nd argument: text string which will be written within this file

#Creates a new file with name and path writefile with content writestr, overwriting any existing file and creating the path if it doesn’t exist.

# Get path from writefile
path=$(dirname "$writefile")

# Check if dir exists else create one
mkdir -p "$path" # make dir if not present
echo "$writestr" > "$writefile" # write str to file

# check if file was created
if [ $? -ne 0 ] #checks if exit status of last command is not equal to 0
then
	echo "Error: File creation"
	exit 1
else
	echo "File $writefile created with string $writestr."
fi
