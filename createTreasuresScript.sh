#!/bin/bash

# script ca sa creez mai multe comori ca sa avem pe ce testa
echo "#### Create treasure data in order to test treasure_hub program ###"
echo " "

echo "Today is " `date`
echo " "

echo "Check if treasure_manager program exists..."

if test -f treasure_manager; then
  ./treasure_manager --add Hunt001 Treasure1 Alex 2.3 45.6 "clue1" 10
  ./treasure_manager --add Hunt002 Treasure1 Andrei 12.3 5.6 "clue2" 16
  ./treasure_manager --add Hunt002 Treasure2 Mirela 33.3 45.6 "clue4" 19
  ./treasure_manager --add Hunt003 Treasure1 Arnoldut 12.3 34.6 "clue5" 15
  ./treasure_manager --add Hunt003 Treasure2 Ilie 13.3 75.6 "sosete" 13
  ./treasure_manager --add Hunt003 Treasure3 Avram 63.3 95.6 "surub" 15
  ./treasure_manager --add Hunt003 Treasure4 Elena 63.3 55.6 "papiota" 55
else
    echo "The treasure_manager programm does not exist"
fi


