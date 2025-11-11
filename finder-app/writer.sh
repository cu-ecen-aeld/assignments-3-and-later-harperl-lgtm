#!/bin/bash

# Script to write a string to a file, creating the file if needed and overwriting existing content.

writefile=$1
writestr=$2

# 1. Check if the number of arguments is correct.
if [ $# -ne 2 ]; then
  echo "Error: two arguments are required!"
  echo "Usage: $0 <directory_path> <search_string>"
  exit 1
fi

# 2. Extract the directory name from the file path
dirpath=$(dirname "$writefile")

# 3. Create directory if needed
mkdir -p "$dirpath"

# 4. Write the string to the file
echo "$writestr" > "$writefile"

# 5. Check echo command return value
if [ $? -ne 0 ]; then
  echo "Error: could not create file ${writefile} with content ${writestr}!"
  exit 1
fi

exit 0
