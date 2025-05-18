#!/bin/bash
echo "Delete all artefacts related to treasure hub"

echo "Deleting all dirs from :" $(pwd)
search_dir=./
for entry in "$search_dir"/*
do
  if [[ -d $entry ]];
  then
  rm -rf $entry
  fi
done

#delete all files that contain "logged_hunt" in their name
echo "Deleting all symbolic links (files that have the text logged_hunt ino their names)"
rm *logged_hunt*

#delete the treasure_hub file resultad from build
echo "Remove the treasure_hub binary file"
rm treasure_hub