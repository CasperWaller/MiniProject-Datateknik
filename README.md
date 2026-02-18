# ESCAPE ROOM

## OVERVIEW
This game is a text-based escape room containing different minigames, puzzles and locks. The map contains nine different rooms with different things to do in each. You have to navigate through the map completing these minigames and puzzles to unlock the doors leading to the escape.

The game runs on the DTEKV Board which uses a RISC-V 32-bit processor

## Game Objectives
The player needs to navigate through the map completing all minigames and puzzles in order to get to the last room. When the player reaches the last room (Elevator), they win.

## Controls
The movement controls consist of switches SW0 (West), SW1(East), SW2(South) and SW3(North).

The button is used to interact with the different rooms and confirm certain decisions.

During the first minigame (Office Memory Challenge) all switches are used. During the boss challenge the button is used to time the reaction. During the vault game the switches SW9, SW8, SW7 and SW6 are used to resemble the code values in binary forms.

## Challenges
### Memory Challenge
In the memory challenge certain LEDs light up for a second and the player is supposed to match the LEDs showing with the switches. If they succed they can continue, if not they have to try again.
### Reaction Challenge
In the reaction challenge the user is supposed to press the button when all LEDs light up. If they pass the time limit they pass the minigame and can continue, if not they have to try again.
### Vault Puzzle
In the vault puzzle a code shows up (both on the terminal and on the 7 segment displays) and the player is supposed to match the values with the binary values using 4 switches, one digit a time.

### Difficulty
In the laboratory (starting room) the user can select the difficulty. This difficulty determines how fast the user has to react in order to pass the challenge.

## Room Layout
List of the rooms with indexes
- Laboratory (0)
- Hallway (1)
- Storage (2)
- Office (3)
- Vault (4)
- Boss Room (5)
- Lounge (6)
- Exit Door (7)
- Elevator (8)

Room Map:

    Office (Challenge ,Key)
       |
    Storage ─── Laboratory ─── Hallway ─── Boss Room (Challenge)
                  (START)          |              |
                                Vault          Lounge (Key)
                              (Puzzle)
                                  |
                              Exit Door
                                  |
                               Elevator
                              (VICTORY!)

## Building the project
### Prerequisites
- RISC-V toolchain
- Make

### Commands
- make
- dtekv-run main.bin

In order to compile and run the program the command make needs to be ran. When this has been done the command dtekv-run main.bin can be executed to start the program.

### Collaborations
This project is a collaboration between @CasperWaller and @aryanmasiul007
