#include <stdint.h>     

// LEDs
#define LEDS (*((volatile unsigned int*) 0x04000000))

// TIMER
#define BASE_TIMER   0x04000020
#define STATUS_TIMER   (*(volatile unsigned int *)(BASE_TIMER + 0x0))
#define CONTROL_TIMER   (*(volatile unsigned int *)(BASE_TIMER + 0x4))
#define PERIOD_L_TIMER   (*(volatile unsigned int *)(BASE_TIMER + 0x8))
#define PERIOD_H_TIMER   (*(volatile unsigned int *)(BASE_TIMER + 0xC))
#define CHECK_L_TIMER   (*(volatile unsigned int *)(BASE_TIMER + 0x10))
#define CHECK_H_TIMER   (*(volatile unsigned int *)(BASE_TIMER + 0x14))

const unsigned char digit27seg[10] = {
  0b11000000, //0
  0b11111001, //1
  0b10100100, //2
  0b10110000, //3
  0b10011001, //4
  0b10010010, //5
  0b10000010, //6
  0b11111000, //7
  0b10000000, //8
  0b10010000, //9
};
int timeoutcount = 0;
static unsigned int pattern_state = 1; // State for pattern generation

extern void delay(int);
extern void display_string(char *str);
extern void time2string(char *str, int time_value);
extern void print(char *str);
extern void print_dec(unsigned int x);
extern void enable_interrupt(void);

// Generate next pattern in sequence
unsigned int generate_pattern() {
    pattern_state = pattern_state * 1103515245 + 12345; //Creates a long sequence without repeating (POSIX/glibc))
    return (pattern_state >> 16) & 0x3FF;
}

typedef struct {
  char *name;
  char *description;
  int north, south, east, west; // index of connected rooms, -1 if no connection
  int locked; // 1 if locked, 0 if unlocked
  int has_key; // 1 if has key, 0 if no key
  int key_collected; // 1 if key collected, 0 if not collected
} Room;

typedef struct {
  int current_room;
  int vault_key; // 0 if no key, 1 if key
  int exitdoor_key; // 0 if no key, 1 if key
  int bossroom_key; // 0 if no key, 1 if key
  int boss_defeated; // 0 if boss not defeated, 1 if boss defeated
  int difficulty_selected; // 1 if selected, 0 if not selected
  int difficulty_level; // 1 if easy, 2 if medium, 3 if hard
  int exit_door_opened; // 0 if door not opened, 1 if door opened
} Player;

Room rooms[9] = {
  {"Laboratory", "Welcome to the escape room. Find keys, solve puzzles, and escape! You are in the laboratory. You can go east to the Hallway or west to the Storage", -1, -1, 1, 2, 0, 0, 0},
  {"Hallway", "You are in the Hallway. You can go south to the Vault, east to the Boss Room or west to the Laboratory", -1, 4, 5, 0, 0, 0, 0},
  {"Storage", "You are in the Storage. You can go north to the office or east to the Laboratory", 3, -1, 0, -1, 0, 0, 0},
  {"Office", "You are in the Office. You can only go south to the Storage", -1, 2, -1, -1, 0, 1, 0},
  {"Vault", "You are in the Vault. You can go north to the Hallway or south to the Exit Door", 1, 7, -1, -1, 1, 0, 0},
  {"Boss Room", "You are in the Boss Room. You can go south to the Lounge or west to the Hallway", -1, 6, -1, 1, 1, 0, 0}, // locked if no bossroom_key
  {"Lounge", "You are in the Lounge. You can only go north to the Boss Room", 5, -1, -1, -1, 0, 1, 0},
  {"Exit Door", "You are in the Exit Door. You can go north to the Vault or south to the Elevator", 4, 8, -1, -1, 1, 0, 0},
  {"Elevator", "You are in the Elevator. You can escape now!", 7, -1, -1, -1, 0, 0, 0},
};

// Player initialization
Player player = {
  .current_room = 0,
  .vault_key = 0,
  .exitdoor_key = 0,
  .bossroom_key = 0,
  .boss_defeated = 0,
  .difficulty_selected = 0, // No difficulty selected
  .difficulty_level = 0, // No level selected
  .exit_door_opened = 0, // Door not opened yet
};

void set_leds(int led_mask) {
  volatile unsigned int* led_pointer = (volatile unsigned int*)0x4000000;

  *led_pointer = led_mask & 0x3FF;
}

void reset_leds() {
  set_leds(0);
}

void set_displays(int display_number, int value) {
    if (display_number < 0 || display_number >= 6 || value < 0 || value > 9) {
        return; // invalid input, do nothing
    }

    // Base address for first display
    volatile unsigned char* display_addr = (volatile unsigned char*)(uintptr_t)(0x04000050 + display_number * 0x10);

    // Write the bit pattern for the digit to the corresponding display
    *display_addr = digit27seg[value];
}

int get_sw(void) {
  volatile unsigned int* switch_address = (volatile unsigned int*)0x04000010;
  return *switch_address & 0x3FF;
}

int get_btn(void) {
  volatile unsigned int* btn_address = (volatile unsigned int*)0x040000d0;
  return *btn_address & 0x1;
}

void display_current_room() {
  Room current = rooms[player.current_room];
  
  // Always show room name and description
  print("\n=== ");
  print(current.name);
  print(" ===\n");
  print(current.description);
  print("\n");
  
  // Check for win condition
  if (player.current_room == 8) { // Elevator
    print("CONGRATULATIONS! YOU ESCAPED!\n");
    delay(2000000);
    print("You successfully completed all challenges!\n");
    delay(2000000);
    print("Memory Challenge: COMPLETED\n");
    delay(2000000);
    print("Reaction Challenge: COMPLETED\n");
    delay(2000000);
    print("Vault Puzzle: COMPLETED\n");
    delay(2000000);
    print("Game Over - You Win!\n\n");
    
    // Exit the game
    while (1) {
      // Infinite loop - game ends here
      delay(1000000);
    }
  }

  else if (player.current_room == 4) { // Vault
    if (!player.exitdoor_key) {
      print("You see a sophisticated vault with glowing 7-segment displays.\n");
      print("Press button to attempt the vault puzzle.\n");
  } else {
      print("You have already solved the vault puzzle.\n");
  }  
  }


  else if (player.current_room == 3) { // Office
      if (current.has_key && !current.key_collected) {
          print("You see a mysterious device on the desk.\n");
          delay(2000000);
          print("Press button to examine it.\n");
      } else if (current.key_collected) {
          print("You have already searched this room.\n");
      }
  }
  else if (player.current_room == 0) { // Laboratory
    if (!player.difficulty_selected) {
      print("Welcome to the Escape Room Laboratory!\n");
      delay(10000000);
      print("Do you want to select your difficulty level?\n");
      delay(10000000);
      print("Press button to select difficulty or explore without selecting.\n");
    } else {
      print("You're back in the laboratory.\n");
      print("Current difficulty: ");
      if (player.difficulty_level == 1) print("Easy (300ms)\n");
      else if (player.difficulty_level == 2) print("Medium (250ms)\n");
      else if (player.difficulty_level == 3) print("Hard (175ms)\n");
    }
  }
  else if (player.current_room == 2) { // Storage
    if (!rooms[2].key_collected) {
      print("You see dusty shelves filled with old equipment.\n");
      delay(10000000);
      print("Press button to search through the shelves.\n");
  } else {      
      print("You have already searched through the shelves.\n");
  }
  }
  else if (player.current_room == 5) { // Boss Room
      if (!player.boss_defeated) {
          print("Press button to accept the boss challenge.\n");
      } else {
          print("You have already defeated the boss.\n");
      }
  } else if (player.current_room == 6) { // Lounge
      if (player.boss_defeated && !rooms[6].key_collected) {
          print("Press button to collect your reward.\n");
      } else if (rooms[6].key_collected) {
          print("You have already collected the reward.\n");
      }
  } else if (player.current_room == 7) { // Exit Door
      print("You see a massive steel door with the exit door key slot.\n");
      print("Press button to insert the key and open the door.\n");
  }

  // Show available exits (but not for Elevator since game ends)
  if (player.current_room != 8) {
    print("You can go: ");
    if (current.north != -1) print("North ");
    if (current.south != -1) print("South ");
    if (current.east != -1) print("East ");
    if (current.west != -1) print("West ");
    print("\n");
  }
}

void go_north() {
  Room current = rooms[player.current_room];
  if (current.north == -1) {
      print("You cannot go north from here.\n");
  } else {
      player.current_room = current.north;
      print("You go north.\n");
      display_current_room();
  }
}

void go_south() {
  Room current = rooms[player.current_room];
  if (current.south == -1) {
      print("You cannot go south from here.\n");
  } else if (current.south == 6 && !player.boss_defeated) { // Lounge requires boss defeated
      print("The door to the south is locked. You must defeat the boss first!\n");
  } else if (current.south == 7 && !player.exitdoor_key) { // Exit Door requires exit door key
      print("The door to the south is locked. You need the exit door key!\n");
  } else if (current.south == 8 && !player.exit_door_opened) { // Elevator requires door to be opened
      print("The door to the south is still closed. You need to open the exit door first!\n");
  } else if (rooms[current.south].locked && !player.vault_key) {
      print("The door to the south is locked. You need the vault key!\n");
  } else {
      player.current_room = current.south;
      print("You go south.\n");
      display_current_room();
  }
}

void go_east() {
  Room current = rooms[player.current_room];
  if (current.east == -1) {
      print("You cannot go east from here.\n");
  } else if (rooms[current.east].locked && !player.bossroom_key) {
      print("The door to the east is locked. You need the boss room key!\n");
  } else {
      player.current_room = current.east;
      print("You go east.\n");
      display_current_room();
  }
}

void go_west() {
  Room current = rooms[player.current_room];
  if (current.west == -1) {
      print("You cannot go west from here.\n");
  } else {
      player.current_room = current.west;
      print("You go west.\n");
      display_current_room();
  }
}

void labinit(void){
  // Reset timeout counter
  timeoutcount = 0;
  
  // Stop timer
  CONTROL_TIMER = 0x0;
  // Clear timeout flag
  STATUS_TIMER = 0;
  
  // 30,000,000 (1 sec) / 1,000 = 30,000 (1 ms)
  PERIOD_L_TIMER = 0x7530;
  PERIOD_H_TIMER = 0x0000;

  // Start timer with continuous + interrupt enable
  CONTROL_TIMER = 0x7;

  enable_interrupt();
}

void handle_interrupt(unsigned cause) {
  if ((STATUS_TIMER) & 0x1) {
    STATUS_TIMER = 0;
    timeoutcount++;
  }
}

int reaction_challenge() {
  print("You see a strange timer device in the center of the room.\n");
  delay(10000000);
  print("Press button if you want to attempt the reaction challenge.\n");
  
  // Wait for player to confirm they want to try
  while (!get_btn()) {
    delay(1000);
  }
  
  // Button detected, wait for release
  while (get_btn()) {
    delay(1000);
  }
  
  delay(100000);
  
  print("Reaction Challenge Instructions:\n");
  delay(5000000);
  print("1. Wait for LEDs to turn on\n");
  delay(5000000);
  print("2. Press button as FAST as possible\n");
  delay(5000000);
  
  // Show difficulty-specific threshold
  if (player.difficulty_level == 1) {
      print("3. You must react faster than 300ms to pass (Easy)\n");
  } else if (player.difficulty_level == 2) {
      print("3. You must react faster than 250ms to pass (Medium)\n");
  } else if (player.difficulty_level == 3) {
      print("3. You must react faster than 175ms to pass (Hard)\n");
  } else {
      print("3. You must react faster than 250ms to pass (Default)\n");
  }
  
  delay(5000000);
  print("4. You can try multiple times if you fail\n");
  delay(5000000);
  print("Press button to start the challenge!\n");
  
  // Wait for player to confirm they're ready
  while (!get_btn()) {
    delay(1000);
  }
  
  // Button detected, wait for release
  while (get_btn()) {
    delay(1000);
  }
  
  delay(100000);
  
  print("Get ready... The challenge will start in 3 seconds!\n");
  
  // Wait for button to be released (prevent cheating)
  while (get_btn()) {
    delay(1000);
  }
  
  // 3 second delay
  delay(30000000);
  
  print("GO!\n");
  
  // Initialize timer
  labinit();
  
  // Set all LEDs on
  set_leds(0x3FF);  // all leds on
  
  // Wait for button press and record the time
  int reaction_time_ms = 0;
  int button_pressed = 0;
  
  while (!button_pressed) {
    if (get_btn()) {
      reaction_time_ms = timeoutcount;
      button_pressed = 1;
    }
  }
  
  // Wait for button release to prevent spamming
  while (get_btn()) {
    delay(1000);
  }
  
  reset_leds();
  
  print("Reaction time: ");
  print_dec(reaction_time_ms);
  print(" ms\n");
  
  int threshold = 250; // Default medium
  if (player.difficulty_level == 1) threshold = 300; // Easy
  else if (player.difficulty_level == 3) threshold = 175; // Hard
  
  // Minimum reaction time to prevent spam cheating (50ms)
  int min_reaction_time = 50;
  
  if (reaction_time_ms < min_reaction_time) {
    delay(10000000);
    print("Suspiciously fast! That's impossible for a human reaction.\n");
    print("You must have cheated. Try again!\n");
    return 0;
  } else if (reaction_time_ms < threshold) {
    delay(10000000);
    print("Excellent! You passed the reaction test!\n");
    return 1;
  } else {
    delay(10000000);
    print("Too slow! You need to react faster than ");
    print_dec(threshold);
    print("ms.\n");
    return 0;
  }
}

static int office_pattern = 0;
int office_memory_challenge() {
  print("Press button if you want to attempt the memory challenge for a key.\n");
  
  // Wait for player to confirm they want to try
  while (!get_btn()) {
    delay(1000);
  }
  
  // Button detected, wait for release
  while (get_btn()) {
    delay(1000);
  }
  
  delay(100000);
  
  print("Memory Challenge Instructions:\n");
  delay(10000000);
  print("1. Watch the LED pattern carefully\n");
  delay(10000000);
  print("2. Set switches SW0-SW9 to match the pattern\n");
  delay(10000000);
  print("3. Press button when you're ready\n");
  delay(10000000);
  print("Press button to start the challenge!\n");
  
  // Wait for player to confirm they're ready
  while (!get_btn()) {
    delay(1000);
  }
  
  // Button detected, wait for release
  while (get_btn()) {
    delay(1000);
  }
  
  delay(100000);
  
  // Generate next pattern in sequence for each attempt
  office_pattern = generate_pattern(); // 10 bit pattern
  print("Memorize this pattern!\n");
  
  print("Showing pattern... \n");
  set_leds(office_pattern);
  delay(10000000);
  reset_leds();
  delay(10000000);
  print("Repeat the pattern using the switches (SW0-SW9), press button when you have finished.\n");

  // Wait for button press and release - more responsive
  while (!get_btn()) {
    delay(1000);
  }
  
  // Button detected, wait for release
  while (get_btn()) {
    delay(1000); // Wait for button release
  }
  
  delay(100000); // Brief debounce
  
  // Get the pattern when button is pressed
  int player_pattern = get_sw() & 0x3FF;
  if (player_pattern == office_pattern) {
    print("Correct! Memory challenge completed!\n");
    return 1; // Success
  } else {
    print("Incorrect! Try again.\n");
    return 0; // Failure
  }
}

int vault_7segment_puzzle_binary() {
  print("You see a vault with 6 glowing 7-segment displays.\n");
  delay(5000000);
  print("The displays show a code: 3-7-2-9-1-5\n");
  delay(5000000);
  print("Enter each digit using switches SW6-SW9 in binary:\n");
  delay(5000000);
  print("SW6=1, SW7=2, SW8=4, SW9=8\n");
  delay(5000000);
  print("Combine switches to make each digit (0-9).\n");
  delay(5000000);
  print("Press button after each digit.\n");
  
  // Display the code on 7-segment displays
  set_displays(5, 3);
  set_displays(4, 7);
  set_displays(3, 2);
  set_displays(2, 9);
  set_displays(1, 1);
  set_displays(0, 5);
  
  delay(50000000); // Show code for 5 seconds
  
  // Clear displays
  for (int i = 0; i < 6; i++) {
      set_displays(i, 0);
  }
  
  int code[] = {5, 1, 9, 2, 7, 3};
  int counter = 0;
  for (int i = 5; i >= 0; i--) {
      counter++;
      print("Enter digit ");
      print_dec(counter);
      print(" (should be ");
      print_dec(code[i]);
      print("): ");
      
      // Wait for player input
      while (!get_btn()) {
          delay(1000);
      }
      while (get_btn()) {
          delay(1000);
      }
      delay(100000);
      
      // Read binary input from SW6-SW9 only
      int switches = get_sw();
      int input = 0;
      
      // Convert binary input to decimal using SW6-SW9
      if (switches & 0x40) input += 1;  // SW6 = 1
      if (switches & 0x80) input += 2;  // SW7 = 2
      if (switches & 0x100) input += 4; // SW8 = 4
      if (switches & 0x200) input += 8; // SW9 = 8
      
      // Show what player entered on display
      set_displays(i, input);
      
      if (input == code[i]) {
          print("Correct!\n");
      } else {
          print("Wrong! The vault remains locked.\n");
          print("You entered: ");
          print_dec(input);
          print(", but needed: ");
          print_dec(code[i]);
          print("\n");
          // Clear all displays
          for (int j = 0; j < 6; j++) {
              set_displays(j, 0);
          }
          return 0;
      }
      delay(2000000);
  }
  
  print("All digits correct! Vault unlocked!\n");
  // Clear displays
  for (int i = 0; i < 6; i++) {
      set_displays(i, 0);
  }
  return 1;
}

void handle_room_interaction() {
  if (player.current_room == 0) { // Laboratory
    if (!player.difficulty_selected) {
      print("=== DIFFICULTY SELECTION ===\n");
      delay(2000000);
      print("Easy: 300ms reaction time - Good for beginners\n");
      delay(2000000);
      print("Medium: 250ms reaction time - Balanced challenge\n");
      delay(2000000);
      print("Hard: 175ms reaction time - Expert level\n");
      delay(2000000);
      print("Use switches to select:\n");
      delay(2000000);
      print("SW9=Easy, SW8=Medium, SW7=Hard\n");
      delay(2000000);
      print("Set ONE switch and press button to confirm.\n");
      
      // Wait for player selection
      while (!get_btn()) {
          delay(1000);
      }
      while (get_btn()) {
          delay(1000);
      }
      
      delay(100000);
      
      int selection = get_sw() & 0x380; // Get switches 7-9 (0x380 = 1110000000)
      
      // Count how many difficulty switches are on
      int diff_count = 0;
      if (selection & 0x200) diff_count++; // SW9
      if (selection & 0x100) diff_count++; // SW8
      if (selection & 0x80) diff_count++;  // SW7
      
      if (diff_count != 1) {
          print("Error: Set ONLY ONE difficulty switch!\n");
          return;
      }
      
      // Check which difficulty was selected
      if (selection & 0x200) { // SW9 (bit 9)
          print("Easy difficulty selected!\n");
          player.difficulty_level = 1;
      } else if (selection & 0x100) { // SW8 (bit 8)
          print("Medium difficulty selected!\n");
          player.difficulty_level = 2;
      } else if (selection & 0x80) { // SW7 (bit 7)
          print("Hard difficulty selected!\n");
          player.difficulty_level = 3;
      }
            
      // Mark difficulty as selected
      player.difficulty_selected = 1;
  } else {
      print("You've already selected your difficulty.\n");
      delay(2000000);
      print("Use the controls to explore and find your way out!\n");
  }
  }
  else if (player.current_room == 4) { // Vault
    if (!player.exitdoor_key) {
      int success = vault_7segment_puzzle_binary();
      if (success) {
          player.exitdoor_key = 1;
          print("You found the exit door key!\n");
      }
    } 
  else {
      print("You have already solved the vault puzzle.\n");
    }
  }
  else if (player.current_room == 2) { // Storage
    if (!rooms[2].key_collected) {
        print("You carefully search through the dusty shelves...\n");
        delay(20000000); // 2 second delay for immersion
        
        print("You find:\n");
        delay(10000000);
        print("- Old circuit boards\n");
        delay(10000000);
        print("- Rusty tools\n");
        delay(10000000);
        print("- Empty containers\n");
        delay(10000000);
        print("- Dusty manuals\n");
        delay(10000000);
        print("Nothing useful here, but you notice a door to the north.\n");
        delay(20000000);
        print("Maybe you should check the office?\n");
        
        // Mark that this room has been searched
        rooms[2].key_collected = 1;
    } else {
        print("You have already searched through the shelves.\n");
        print("There's nothing more to find here.\n");
    }
  } 
    else if (player.current_room == 3) { // Office
      if (rooms[3].has_key && !rooms[3].key_collected) {
          int success = office_memory_challenge();
          
          if (success) {
              player.bossroom_key = 1;
              print("You found the boss room key!\n");
              rooms[3].key_collected = 1;
          } else {
              print("Try the memory challenge again when you're ready.\n");
          }
      } else if (rooms[3].key_collected) {
          print("You have already searched this room.\n");
      } else {
          print("Nothing to interact with here.\n");
      }
  } else if (player.current_room == 5) { // Boss Room
      if (!player.boss_defeated) {
          if (!player.bossroom_key) {
              print("You need the boss room key to enter this challenge!\n");
              print("Find the key in another room first.\n");
          } else {
              print("You use the boss room key and enter the challenge!\n");
              delay(10000000);
              int success = reaction_challenge();
              
              if (success) {
                  player.boss_defeated = 1;
                  print("Boss defeated! You can now continue to the lounge.\n");
              } else {
                  print("Try the reaction challenge again when you're ready.\n");
              }
          }
      } else {
          print("You have already defeated the boss.\n");
      }
  } else if (player.current_room == 6) { // Lounge
      if (player.boss_defeated && !rooms[6].key_collected) {
          print("You found the vault key!\n");
          player.vault_key = 1;
          rooms[6].key_collected = 1;
      } else if (!player.boss_defeated) {
          print("You need to defeat the boss first to get a reward here.\n");
      } else if (rooms[6].key_collected) {
          print("You have already collected the vault key.\n");
      } else {
          print("Nothing to interact with here.\n");
      }
  } else if (player.current_room == 7) { // Exit Door
      if (!player.exit_door_opened) {
          print("You insert the exit door key...\n");
          delay(2000000);
          print("The door creaks ominously...\n");
          delay(2000000);
          print("With a loud CLANG, the massive door slides open.\n");
          delay(2000000);
          print("A rush of cold air hits your face.\n");
          delay(2000000);
          print("The elevator awaits beyond.\n");
          delay(2000000);
          print("The exit door is now open! You can proceed to the elevator.\n");
          player.exit_door_opened = 1;
      } else {
          print("The exit door is already open. You can proceed to the elevator.\n");
      }
  } else {
      print("Nothing to interact with in this room.\n");
  }
}

int main() {
  print("Welcome to the Escape Room Game!\n");
  delay(15000000);
  print("MOVEMENT: Set ONE switch + press button\n");
  delay(20000000);
  print("INTERACT: Turn OFF all switches + press button\n");
  delay(20000000);
  print("SW0=West, SW1=East, SW2=South, SW3=North\n\n");
  delay(20000000);
  // Display starting room immediately
  display_current_room();
  
  int last_btn_state = 0;
  
  while (1) {
      // Read current input states
      int current_sw = get_sw();
      int current_btn = get_btn();
      
      // Detect button press (edge detection)
      int button_pressed = current_btn && !last_btn_state;
      
      if (button_pressed) {
          // Check how many switches are on
          int switch_count = 0;
          if (current_sw & 0x1) switch_count++; // West
          if (current_sw & 0x2) switch_count++; // East
          if (current_sw & 0x4) switch_count++; // South
          if (current_sw & 0x8) switch_count++; // North
          
          if (switch_count == 0) {
              // No switches - room interaction
              handle_room_interaction();
          }
          else if (switch_count == 1) {
              // Exactly one switch - movement
              if (current_sw & 0x1) {
                  go_west();
              } else if (current_sw & 0x2) {
                  go_east();
              } else if (current_sw & 0x4) {
                  go_south();
              } else if (current_sw & 0x8) {
                  go_north();
              }
          } else {
              // Multiple switches - error
              print("ERROR: Turn on only ONE switch for movement!\n");
              delay(10000000);
              print("Or turn OFF all switches to interact with room.\n");
          }
          
          delay(1000000); // Debounce delay
      }
      
      // Update previous button state
      last_btn_state = current_btn;
      delay(10000);
  }
}