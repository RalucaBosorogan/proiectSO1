# proiectSO
Proiect Sisteme de Operare, Raluca Bosorogan

Comanda compilare: gcc -o treasure_manager ./treasure_manager.c
Comanda rulare: ./treasure_manager --<command> <hunt_id> [additional arguments]
   commands:
       --add <hunt_id>: Add a new treasure to the specified hunt (game session).
                        Each hunt is stored in a separate directory. 
       --list <hunt_id>: List all treasures in the specified hunt.
                         First print the hunt name, the (total) file size and last modification time of its treasure file(s), then list the treasures.
       --view <hunt_id> <id>: View details of a specific treasure
       --remove_treasure <hunt_id> <id>: Remove a treasure
       --remove_hunt <hunt_id>: Remove an entire hunt

Exemplu:
 ./treasure_manager --add Hunt01 Treasure1 Alex 2.3 45.6 "clue" 10
Treasure added successfully.

  ./treasure_manager --add Hunt01 Treasure1 Andrei 12.3 5.6 "clue2" 16
Treasure added successfully.

  ./treasure_manager --add Hunt01 Treasure2 Mirela 33.3 45.6 "clue4" 19
Treasure added successfully.

  ./treasure_manager --add Hunt02 Treasure1 Arnoldut 12.3 34.6 "clue5" 15
Treasure added successfully.

  ./treasure_manager --add Hunt02 Treasure2 HanSolo 23.3 45.6 "clue6" 13
Treasure added successfully.
 
  ./treasure_manager --list Hunt01
Hunt: Hunt01
Size: 1092 bytes
Last modified: Sun Apr 13 12:55:58 2025
- ID: Treasure1, User: Alex, Location: (2.30, 45.60), Value: 10
- ID: Treasure1, User: Andrei, Location: (12.30, 5.60), Value: 16
- ID: Treasure2, User: Mirela, Location: (33.30, 45.60), Value: 19
 
 ./treasure_manager --view Hunt01 Treasure1
ID: Treasure1
User: Alex
Lat: 2.30
Long: 45.60
Clue: clue
Value: 10
 

  ./treasure_manager --remove_treasure Hunt01 Treasure2
Treasure removed.
  ./treasure_manager --list Hunt01
Hunt: Hunt01
Size: 728 bytes
Last modified: Sun Apr 13 12:56:01 2025
- ID: Treasure1, User: Alex, Location: (2.30, 45.60), Value: 10
- ID: Treasure1, User: Andrei, Location: (12.30, 5.60), Value: 16
 
  ./treasure_manager --remove_hunt Hunt01 
Hunt 'Hunt01' removed.
  ./treasure_manager --remove_hunt Hunt02  
Hunt 'Hunt02' removed.
  ./treasure_manager --remove_hunt Hunt03
Hunt 'Hunt03' removed.
  
