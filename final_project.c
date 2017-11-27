/*********************************************
 * final_project.c                           *
 * 11/21/2017                                *
 * ES52 Final Project: Miniature Guitar Hero *
 * Andrew Shackelford and Vivian Qiang       *
 * ashackelford@college.harvard.edu          *
 * vqiang@college.harvard.edu                *
 * This program controls an Arduino Micro    *
 * to run a minature Guitar Hero style game  *
 * using 32 LEDs and 4 push-buttons.         *
 ********************************************/

// music libraries
#include <pcmConfig.h>
#include <pcmRF.h>
#include <TMRpcm.h>
TMRpcm song;

// SD libraries
#include <SD.h>
#include <SPI.h>
#define SD_SELECT SS

// NOTE PINS
#define NOTE_SDO 5
#define NOTE_CLK 6
#define NOTE_LOAD 12

// SCORE PINS
#define SCORE_CLK 4
#define SCORE_SDO 0
#define SCORE_LOAD 1

// global variables for keeping track of songs and notes
const byte NUM_SONGS = 1;
int score = 0;
byte song_num = 0;
long song_start_time = 0;
long cur_note_time = 0;
bool song_playing = true;

int note_index;
byte first_notes[8000][4];
byte second_notes[7000][4];
bool notes[4][8];
bool past_notes[4][8];
File noteFile;

// ISR variables
volatile bool should_change_song = false;
volatile bool should_turn_on_off = false;

/* helper functions */

bool getNthBitOfNum(int num, byte n) {
  return (num >> n) & 1;
}

/***********************************
 * converts a byte to a bool array *
 ***********************************/
void byteToArr(unsigned char c, bool b[8])
{
  for (byte i = 0; i < 8; i++) {
    b[i] = (c & (1<<i)) != 0;
  }
}

/*************************************
 * set the given pin range to output *
 *************************************/
void setOutputPinRange(byte min, byte max) {
  for (byte i = min; i <= max; i++) {
    pinMode(i, OUTPUT);
  }
}

void setPinLow(byte pin) {
  digitalWrite(pin, LOW);
}

void setPinHigh(byte pin) {
  digitalWrite(pin, HIGH);
}

void sendClock(byte pin) {
  setPinLow(pin);
  setPinHigh(pin);
}

/* ISRs */

void turnOnOffISR() {
  should_turn_on_off = true;
}

/****************************************
 * turn all displays and outputs on/off *
 ****************************************/
void turnOnOff() {
  should_turn_on_off = false;
  if (song_playing) {
    song.stopPlayback();
    score = 0;
    note_index = 0;
    song_num -= 1;
  } else {
    changeSong();
  }
  song_playing = !song_playing;
}

void changeSongISR() {
  should_change_song = true;
}

/***************************
 * change to the next song *
 ***************************/
void changeSong() {
  should_change_song = false;
  // stop playing song, reset score data, update song display
  song.stopPlayback();
  song_num += 1;
  if (song_num > NUM_SONGS) {
    song_num = 1;
  }
  score = 0;
  sendSongData(song_num);
  loadNoteData();

  // start playing next song
  char str[8];
  sprintf(str, "%d", song_num);
  strcat(str, ".wav");
  song.play(str);

  // update times
  song_start_time = millis();
  cur_note_time = song_start_time;
  note_index = 0;
}

/* note data loading/updating */

void loadNoteData() {
  char str[8];
  sprintf(str, "%d", song_num);
  strcat(str, ".txt");
  noteFile = SD.open(str);
  byte index = 0;
  for (int i = 0; i < 8000; i++) {
    if (noteFile.available()) {
      first_notes[i][index] = noteFile.read();
    } else {
      first_notes[i][index] = 0x00;
    } 
    index++;
    if (index > 3) {
        index = 0;
    }
  }
  for (int i = 8000; i < 15000; i++) {
    if (noteFile.available()) {
      second_notes[i][index] = noteFile.read();
    } else {
      second_notes[i][index] = 0x00;
    } 
    index++;
    if (index > 3) {
        index = 0;
    }
  }
}

void updateNoteData() {
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 8; j++) {
      past_notes[i][j] = notes[i][j];
    }
    if (note_index < 8000) {
      byteToArr(first_notes[note_index][i], notes[i]);
    } else {
      byteToArr(second_notes[note_index][i], notes[i]);
    }
  }
}

/* note data sending */

/**************************************************
 * sends out note data to 2 8-bit shift registers *
 * for a 4x8 multiplexed LED array                *
 **************************************************/
void sendNoteData() {
  for (byte x = 0; x < 4; x++) {

    // send row data
    for (byte r = 0; r < 4; r++) {
      if (r == x) {
        setPinHigh(NOTE_SDO);
      } else {
        setPinLow(NOTE_SDO);
      }
      sendClock(NOTE_CLK);
    }

    // send column data
    for (byte y = 0; y < 8; y++) {
      if (notes[x][y]) {
        setPinHigh(NOTE_SDO);
      } else {
        setPinLow(NOTE_SDO);
      }
      sendClock(NOTE_CLK);
    }
    sendClock(NOTE_LOAD);
    delay(1); // wait 1 ms for persistence of vision
    
  }
}

/* score data sending */

/**********************************************
 * send out score data to the 3-digit display *
 **********************************************/
void sendScoreData() {
  for (byte i = 1; i <= 3; i++) {
    // send low bits to preface digit address
    setPinLow(SCORE_SDO);
    for (byte x = 0; x < 6; x++) {
      sendClock(SCORE_CLK);
    }
    
    // send digit address
    switch(i) {
      case 1:
        sendClock(SCORE_CLK);
        sendClock(SCORE_CLK);
        break;
      case 2:
        sendClock(SCORE_CLK);
        setPinHigh(SCORE_SDO);
        sendClock(SCORE_CLK);
        break;
      case 3:
        setPinHigh(SCORE_SDO);
        sendClock(SCORE_CLK);
        setPinLow(SCORE_SDO);
        sendClock(SCORE_CLK);
        break; 
    }

    // send digit data
    byte score_digit = score / (10*i);
    for (byte j = 7; j >= 0; j--) {
      if (getNthBitOfNum(score_digit, j)) {
        setPinHigh(SCORE_SDO);
        sendClock(SCORE_CLK);
      } else {
        setPinLow(SCORE_SDO);
        sendClock(SCORE_CLK);
      }
    }
    sendClock(SCORE_LOAD);
  }
}

/* song data sending */

/********************************************
 * send song number to the Numitron display *
 ********************************************/
void sendSongData(int song_num) {
  for (byte i = 0; i < 4; i++) {
    if (getNthBitOfNum(song_num, i)) {
      setPinHigh(i + 13);
    } else {
      setPinLow(i + 13);
    }
  }
}

/* note checking */

/****************************************
 * update the note reference output for *
 * comparison to user input             *
 ****************************************/
void updateNoteReference() {
  // TODO
}

/*****************************************
 * compare combination of user input and *
 * reference output to see if user       *
 * hit or missed note                    *
 *****************************************/
bool checkNoteHit() {
  return false; // TODO
}


void setup() {
  // set our output pins
  setOutputPinRange(0, 1);
  setOutputPinRange(4, 13);

  // attach interrupts to on/off and song sel inputs
  attachInterrupt(digitalPinToInterrupt(2), turnOnOffISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), changeSongISR, RISING);

  // initialize SD card
  Serial.begin(9600);
  Serial.print("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default. // Note that even if it's not used as the CS pin, the hardware SS pin
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output // or the SD library functions will not work.
  pinMode(SD_SELECT, OUTPUT);
  if (!SD.begin(SD_SELECT)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  song.speakerPin = 9;
  changeSong();
}

void loop() {
  if (should_turn_on_off) {
    turnOnOff();
  }
  if (should_change_song) {
    changeSong();
  }
  if (song_playing) {
    if (millis() > cur_note_time+25) {
      // every 25 ms, send out a new frame of note data
      cur_note_time += 25;
      note_index++;
      updateNoteData();
      updateNoteReference();
      if (checkNoteHit()) {
        score += 1;
      }
    }
    sendNoteData();
    sendScoreData();
  }
}