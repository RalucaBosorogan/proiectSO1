👨‍💻 Autor
Boșorogan Raluca Ștefania
Proiect realizat pentru cursul Sisteme de Operare

**Faza 1:** Gestionarea comorilor prin fișiere binare și directoare.
**Faza 2:** Monitorizarea și comunicarea cu jucători prin semnale și pipe-uri.
**Faza 3:** Integrare cu un proces extern pentru scoruri.

⚙️ Funcționalități:

**Faza 1** – treasure_manager

| Comandă                          | Descriere                         |
| -------------------------------- | --------------------------------- |
| --add <hunt_id>                  | Adaugă o comoară într-o vânătoare |
| --list <hunt_id>                 | Listează toate comorile           |
| --view <hunt_id> <treasure_id>   | Afișează o comoară după ID        |
| --delete <hunt_id> <treasure_id> | Șterge o comoară după ID          |

💡 Comanda compilare: gcc -o treasure_manager ./treasure_manager.c
💡 Comanda rulare: ./treasure_manager --<command> <hunt_id> [additional arguments]

🗃️ Exemplu 🗃️:

 ➕  ./treasure_manager --add Hunt01 Treasure1 Alex 2.3 45.6 "clue" 10
Treasure added successfully.

 ➕  ./treasure_manager --add Hunt01 Treasure1 Andrei 12.3 5.6 "clue2" 16
Treasure added successfully.

 ➕  ./treasure_manager --add Hunt01 Treasure2 Mirela 33.3 45.6 "clue4" 19
Treasure added successfully.

 ➕  ./treasure_manager --add Hunt02 Treasure1 Arnoldut 12.3 34.6 "clue5" 15
Treasure added successfully.

 ➕  ./treasure_manager --add Hunt02 Treasure2 HanSolo 23.3 45.6 "clue6" 13
Treasure added successfully.
 
 📋  ./treasure_manager --list Hunt01
Hunt: Hunt01
Size: 1092 bytes
Last modified: Sun Apr 13 12:55:58 2025
- ID: Treasure1, User: Alex, Location: (2.30, 45.60), Value: 10
- ID: Treasure1, User: Andrei, Location: (12.30, 5.60), Value: 16
- ID: Treasure2, User: Mirela, Location: (33.30, 45.60), Value: 19
 
 🔍 ./treasure_manager --view Hunt01 Treasure1
ID: Treasure1
User: Alex
Lat: 2.30
Long: 45.60
Clue: clue
Value: 10

❌  ./treasure_manager --remove_treasure Hunt01 Treasure2
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



**Faza 2** - treasure_hub

->Creează un proces monitor care comunică cu jucători (procese copil)
->Procesele copil primesc semnale pentru start/stop/score
->Pipe-uri sunt folosite pentru trimiterea comenzilor între procese

💡Comanda compilare:  gcc -o treasure_hub treasure_hub.c 
💡Comanda rulare: ./treasure_hub 


📌commands:

    🟢👀 start_monitor, 
    📜🗺️ list_hunts,
     💎📋 list_treasures,
      🔍💰 view_treasure,
       🔴 stop_monitor,
        🚪🏁 exit


**Faza 3** - Integrare scoruri

->Un program extern (score_calculator) este invocat pentru a evalua scorurile jucătorilor.
->Comunicarea are loc prin pipe-uri redirecționate (dup2, execvp).
->Rezultatele sunt colectate de procesul principal și afișate în terminal.

Exemplu: 

💡Comanda compilare:  gcc -o treasure_hub treasure_hub.c 
💡Comanda rulare: ./treasure_hub 

 📌commands:
    start_monitor, list_hunts, list_treasures, view_treasure, calculate_score, stop_monitor, exit

> calculate_score
Calculate the score for each hunt
The score of the hunt "Hunt001" is 10!
The score of the hunt "Hunt003" is 98!
The score of the hunt "Hunt002" is 35!






























  
