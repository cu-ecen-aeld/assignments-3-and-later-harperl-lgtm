#!/bin/bash

# Script to find a string in files within given directory

filesDir=$1
searchStr=$2

# 1. Check if the number of arguments is correct.
if [ $# -ne 2 ]; then
  echo "Error: two arguments are required!"
  echo "Usage: $0 <directory_path> <search_string>"
  exit 1
fi

# 2. Check if filesDir is a valid directory
if [ ! -d "$filesDir" ]; then
  echo "Error: The first argument does not represent a directory!"
  exit 1
fi

# 3. Find the number of files under the given directory
num_files=$(find "$filesDir" -type f | wc -l | xargs)

# 4. Find the number of lines containing the searchStr
num_matching_lines=$(grep -r "$searchStr" "$filesDir" | wc -l | xargs)

# 5. Print the final results
echo "The number of files are ${num_files} and the number of matching lines are ${num_matching_lines}"

exit 0