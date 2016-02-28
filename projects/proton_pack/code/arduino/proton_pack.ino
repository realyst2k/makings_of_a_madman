#include <FatReader.h>
#include <SdReader.h>
#include <avr/pgmspace.h>
#include "WaveUtil.h"
#include "WaveHC.h"


SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the filesystem on the card
FatReader f;      // This holds the information for the file we're play

boolean pack_running = 0;
boolean wand_state = 0;

WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

#define DEBOUNCE 30  // button debouncer

// this handy function will return the number of bytes currently free in RAM, great for debugging!   
int freeRam(void)
{
  extern int  __bss_end; 
  extern int  *__brkval; 
  int free_memory; 
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end); 
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval); 
  }
  return free_memory; 
} 

void sdErrorCheck(void)
{
  if (!card.errorCode()) return;
  putstring("\n\rSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  putstring(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}

void setup() {
  // set up serial port
  Serial.begin(9600);
  putstring_nl("WaveHC with 6 buttons");
  
   putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(freeRam());      // if this is under 150 bytes it may spell trouble!
  
  // Set pin to connect to optocoupler to control LED chaser
  pinMode(6,OUTPUT);
  digitalWrite(6, LOW);

  // Set pin to connect to 5V optocoupler
  pinMode(9,OUTPUT);
  digitalWrite(9, LOW);
  
  // Set pin to connect to optocoupler to control wand LED
  pinMode(7,OUTPUT);
  digitalWrite(7, LOW);
  
  // Set the output pins for the DAC control. This pins are defined in the library
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
 
  // pin13 LED
  pinMode(13, OUTPUT);
 
  // enable pull-up resistors on switch pins (analog inputs)
  digitalWrite(14, HIGH);
  digitalWrite(15, HIGH);
  digitalWrite(16, HIGH);
  digitalWrite(17, HIGH);
  digitalWrite(18, HIGH);
  digitalWrite(19, HIGH);
 
  //  if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
  if (!card.init()) {         //play with 8 MHz spi (default faster!)  
    putstring_nl("Card init. failed!");  // Something went wrong, lets print out why
    sdErrorCheck();
    while(1);                            // then 'halt' - do nothing!
  }
  
  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);
 
// Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {     // we have up to 5 slots to look in
    if (vol.init(card, part)) 
      break;                             // we found one, lets bail
  }
  if (part == 5) {                       // if we ended up not finding one  :(
    putstring_nl("No valid FAT partition!");
    sdErrorCheck();      // Something went wrong, lets print out why
    while(1);                            // then 'halt' - do nothing!
  }
  
  // Lets tell the user about what we found
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(),DEC);     // FAT16 or FAT32?
  
  // Try to open the root directory
  if (!root.openRoot(vol)) {
    putstring_nl("Can't open root dir!"); // Something went wrong,
    while(1);                             // then 'halt' - do nothing!
  }
  
  // Whew! We got past the tough parts.
  putstring_nl("Ready!");
}

void loop() {
  //putstring(".");            // uncomment this to see if the loop isnt running
  int switch_state = read_switches();
  char switch_debug[25];
  sprintf(switch_debug,"Switch state: %d",switch_state);
  Serial.println(switch_debug);
  switch (read_switches()) {
    case 0:
      if(pack_running) {
        if(!wand_state) {
          digitalWrite(7, HIGH);
          playcomplete("WAND1.WAV");
          wand_state = true;
        }
        digitalWrite(7, HIGH);
        playcomplete("WAND2.WAV");
      }
      break;
    case 2:
      if(wand_state) {
         wand_state = false;
         digitalWrite(7, LOW); 
      }
      if(!pack_running) {
        playcomplete("START.WAV");
        pack_running = true;
      }
      playcomplete("RUN.WAV");
      digitalWrite(6, HIGH);
      digitalWrite(9, HIGH);
      break;
    case 3:
      if(pack_running) {
        playcomplete("STOP.WAV");
        pack_running = false;
        wand_state = false;
        digitalWrite(6, LOW);
        digitalWrite(9, LOW);
      }
  }
}

int read_switches() {
   byte packswitch  = digitalRead(14);
   byte wandbutton  = digitalRead(15);
   // Combine into a single int to use in a switch statement
   // packswitch    wandbutton    state
   //     0             0           0
   //     0             1           1
   //     1             0           2
   //     1             1           3
   //
   int state = (packswitch << 1) | wandbutton; 
   return state;
}

byte check_switches()
{
  static byte previous[6];
  static long time[6];
  byte buttonstates[] = {0,0};
  // buttonstates[0]  pack button
  // buttonstates[1]  wand button
  byte reading;
  byte pressed;
  byte index;
  pressed = 0;

  for (byte index = 0; index < 2; ++index) {
    reading = digitalRead(14 + index);
    if (reading == LOW && previous[index] == HIGH && millis() - time[index] > DEBOUNCE)
    {
      // switch pressed
      time[index] = millis();
      buttonstates[index] = 1;
      break;
    }
    previous[index] = reading;
  }
  // return switch number (1 - 6)
  int state = (buttonstates[0] << 1) | buttonstates[1];
  return (state);
}

// Plays a full file from beginning to end with no pause.
void playcomplete(char *name) {
  // call our helper to find and play this name
  playfile(name);
  while (wave.isplaying) {
  // do nothing while its playing
  }
  // now its done playing
}

void playfile(char *name) {
  // see if the wave object is currently doing something
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  // look in the root directory and open the file
  if (!f.open(root, name)) {
    putstring("Couldn't open file "); Serial.print(name); return;
  }
  // OK read the file and turn it into a wave object
  if (!wave.create(f)) {
    putstring_nl("Not a valid WAV"); return;
  }
  
  // ok time to play! start playback
  wave.play();
}
