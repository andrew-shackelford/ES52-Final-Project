/*********************************************
 * final_project.c                           *
 * 11/21/2017                                *
 * ES52 Final Project: Miniature Guitar Hero *
 * Andrew Shackelford and Vivian Qiang       *
 * ashackelford@college.harvard.edu          *
 * vqiang@college.harvard.edu                *
 * This program controls an Arduino Due      *
 * to run a minature Guitar Hero style game  *
 * using 32 LEDs and 4 push-buttons.         *
 *********************************************/

// libraries
#include <SD.h>
#include <SPI.h>
#include <Audio.h>
#include <DueTimer.h>

// SD PIN
#define SD_SELECT 53

// NOTE PINS
#define NOTE_SDO 48
#define NOTE_CLK 36
#define NOTE_LOAD 46

// SCORE PINS
#define SCORE_CLK 4
#define SCORE_SDO 0
#define SCORE_LOAD 1

// INT PINS
#define CHANGE_SONG_INT_PIN 49
#define NOTE_INT_PIN 47

// VOLTAGE REFERENCES
#define NOTE_REF_OUT 2
const int REFERENCES[4] = {144, 97, 69, 34}; // out of 255

// CONSTANTS
const int NUM_SONGS = 7;
const int BUFFER_SIZE = 1024; // number of audio samples to read
const int VOLUME = 1024;

// global variables for keeping track of songs
int score = 0;
int song_num = 0;
bool song_playing = true;

// global variables for keeping track of notes
bool notes[4][8];
long cur_frame_time = 0;
int column = 0;

// global variables for keeping track of files / audio
short audio_buffer[BUFFER_SIZE];
File note_file;
File music_file;

// ISR variables
volatile bool should_change_song = false;
volatile bool should_check_note_hit = false;

/* helper functions */

/***********************************
 * converts a byte to a bool array *
 ***********************************/
void byteToArr(unsigned char c, bool b[8])
{
  for (int i = 0; i < 8; i++) {
    b[i] = (c & (1<<i)) != 0;
  }
}

void setPinLow(int pin) {
  digitalWrite(pin, LOW);
}

void setPinHigh(int pin) {
  digitalWrite(pin, HIGH);
}

void sendClock(int pin) {
  setPinLow(pin);
  setPinHigh(pin);
}

/* ISRs */

void changeSongISR() {
  should_change_song = true;
}

void checkNoteISR() {
  should_check_note_hit = true;
}

/***************************
 * change to the next song *
 ***************************/
void changeSong() {
  // update ISR flag
  should_change_song = false;

  // close files
  music_file.close();
  note_file.close();

  // update song num, reset score data
  song_num += 1;
  if (song_num > NUM_SONGS) {
    song_num = 1;
  }
  score = 0;

  // load in notes and new song
  loadNoteData();
  loadSongData();

  // update start time
  cur_frame_time = millis();
  song_playing = true;
}

/*****************************************
 * compare combination of user input and *
 * reference output to see if user       *
 * hit or missed note                    *
 *****************************************/
void checkNoteHit() {
  should_check_note_hit = false;
  if (!digitalRead(NOTE_INT_PIN)) {
    // if the button(s) pressed are correct
    if (notes[0][7] || notes[1][7] || notes[2][7] || notes[3][7]) {
      // the player has pressed the button correctly,
      // increment the score
      score++;
    } else {
      // the button press is correct but there are no notes,
      // so don't change the score
    }
  } else {
    // the button(s) pressed are not correct
    if (notes[0][6] || notes[1][6] || notes[2][6] || notes[3][6] ||
        notes[0][7] || notes[1][7] || notes[2][7] || notes[3][7]) {
      // the player pressed or released the button a bit early,
      // but within the margin of error so it's all good
      // (this also occurs when the player simply misses a note,
      // but we will simply not give them a point instead of
      // penalizing them a point)
    } else {
      // the player pressed the button when there was not a note,
      // so penalize them a point
      score--;
    }
  }
}

/* song data loading */

void loadSongData() {
  char str[8];
  sprintf(str, "%d", song_num);
  strcat(str, ".wav");

  Audio.begin(44100, 100);
  music_file = SD.open(str);
}

/* note data loading/updating */

void loadNoteData() {
  char str[8];
  sprintf(str, "%d", song_num);
  strcat(str, ".txt");
  note_file = SD.open(str);
  updateNoteData();
}

void updateNoteData() {
  for (int i = 0; i < 4; i++) {
    if (note_file.available()) {
      byteToArr(note_file.read(), notes[i]);
    } else {
      byteToArr(0x00, notes[i]);
    }
  }
}

/* serial data sending */

void sendData() {
  // reset column
  if (column > 3) {
    column = 0;
  }

  // send note data
  sendNoteData();

  // send score data
  switch (column) {
    case 0:
      sendDigit(score % 100, column);
      break;
    case 1:
      sendDigit((score/10) % 10, column);
      break;
    case 2:
      sendDigit(score/100, column);
      break;
  }

  // increment column
  column++;
}


/**************************************************
 * sends out note data to 2 8-bit shift registers *
 * for a 4x8 multiplexed LED array                *
 **************************************************/
void sendNoteData() {
  // send LED row data
  for (int r = 0; r < 4; r++) {
    if (r == column) {
      setPinHigh(NOTE_SDO);
    } else {
      setPinLow(NOTE_SDO);
    }
    sendClock(NOTE_CLK);
  }

  // send LED column data
  for (int y = 0; y < 8; y++) {
    if (notes[3-column][7-y]) {
      setPinHigh(NOTE_SDO);
    } else {
      setPinLow(NOTE_SDO);
    }
    sendClock(NOTE_CLK);
  }

  sendClock(NOTE_LOAD);
}

/********************************************************
 * sends out score data to 2 8-bit shift registers for  *
 * a 3-digit 7-segment multiplexed display              *
 ********************************************************/
void sendDigit(int n, int d) {
  // send digit data
  setPinLow(SCORE_SDO);
  for (int i = 2; i > d; i--) {
    sendClock(SCORE_CLK);
  }

  setPinHigh(SCORE_SDO);
  sendClock(SCORE_CLK);

  setPinLow(SCORE_SDO);
  for (int i = 0; i < d; i++) {
    sendClock(SCORE_CLK);
  }

  // send number data
  bool b[8];
  intToSevenSegment(n, b);
  for (int i = 7; i >= 0; i--) {
    if (b[i]) {
      setPinHigh(SCORE_SDO);
    } else {
      setPinLow(SCORE_SDO);
    }
    sendClock(SCORE_CLK);
  }

  sendClock(SCORE_LOAD);
}

/************************************************************
 * convert a one-digit integer to a 7-segment boolean array *
 ************************************************************/
void intToSevenSegment(int n, bool b[8]) {
  // active low
  switch (n) {
    case 0:
      b[0] = 0; // a
      b[1] = 0; // b
      b[2] = 0; // c
      b[3] = 0; // d
      b[4] = 0; // e
      b[5] = 0; // f
      b[6] = 1; // g
      b[7] = 1; // dp
      break;
    case 1:
      b[0] = 1; // a
      b[1] = 0; // b
      b[2] = 0; // c
      b[3] = 1; // d
      b[4] = 1; // e
      b[5] = 1; // f
      b[6] = 1; // g
      b[7] = 1; // dp
      break;
    case 2:
      b[0] = 0; // a
      b[1] = 0; // b
      b[2] = 1; // c
      b[3] = 0; // d
      b[4] = 0; // e
      b[5] = 1; // f
      b[6] = 0; // g
      b[7] = 1; // dp
      break;
    case 3:
      b[0] = 0; // a
      b[1] = 0; // b
      b[2] = 0; // c
      b[3] = 0; // d
      b[4] = 1; // e
      b[5] = 1; // f
      b[6] = 0; // g
      b[7] = 1; // dp
      break;
    case 4:
      b[0] = 1; // a
      b[1] = 0; // b
      b[2] = 0; // c
      b[3] = 1; // d
      b[4] = 1; // e
      b[5] = 0; // f
      b[6] = 0; // g
      b[7] = 1; // dp
      break;
    case 5:
      b[0] = 0; // a
      b[1] = 1; // b
      b[2] = 0; // c
      b[3] = 0; // d
      b[4] = 1; // e
      b[5] = 0; // f
      b[6] = 0; // g
      b[7] = 1; // dp
      break;
    case 6:
      b[0] = 0; // a
      b[1] = 1; // b
      b[2] = 0; // c
      b[3] = 0; // d
      b[4] = 0; // e
      b[5] = 0; // f
      b[6] = 0; // g
      b[7] = 1; // dp
      break;
    case 7:
      b[0] = 0; // a
      b[1] = 0; // b
      b[2] = 0; // c
      b[3] = 1; // d
      b[4] = 1; // e
      b[5] = 1; // f
      b[6] = 1; // g
      b[7] = 1; // dp
      break;
    case 8:
      b[0] = 0; // a
      b[1] = 0; // b
      b[2] = 0; // c
      b[3] = 0; // d
      b[4] = 0; // e
      b[5] = 0; // f
      b[6] = 0; // g
      b[7] = 1; // dp
      break;
    case 9:
      b[0] = 0; // a
      b[1] = 0; // b
      b[2] = 0; // c
      b[3] = 1; // d
      b[4] = 1; // e
      b[5] = 0; // f
      b[6] = 0; // g
      b[7] = 1; // dp
      break;
    default:
      b[0] = 1; // a
      b[1] = 1; // b
      b[2] = 1; // c
      b[3] = 1; // d
      b[4] = 1; // e
      b[5] = 1; // f
      b[6] = 1; // g
      b[7] = 1; // dp
      break;
  }
}

/* note reference */

/****************************************
 * update the note reference output for *
 * comparison to user input             *
 ****************************************/
void updateNoteReference() {
  int out = 0;
  for (int i = 0; i < 4; i++) {
    if (notes[i][7]) {
      out += REFERENCES[i];
    }
  }
  analogWrite(NOTE_REF_OUT, out);
}


void setup() {
  // set our output pins
  pinMode(SD_SELECT, OUTPUT);
  pinMode(NOTE_REF_OUT, OUTPUT);

  pinMode(NOTE_SDO, OUTPUT);
  pinMode(NOTE_CLK, OUTPUT);
  pinMode(NOTE_LOAD, OUTPUT);

  pinMode(SCORE_SDO, OUTPUT);
  pinMode(SCORE_CLK, OUTPUT);
  pinMode(SCORE_LOAD, OUTPUT);

  // set our input pins
  pinMode(NOTE_INT_PIN, INPUT);
  pinMode(CHANGE_SONG_INT_PIN, INPUT);

  // attach interrupts
  attachInterrupt(digitalPinToInterrupt(NOTE_INT_PIN), checkNoteISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CHANGE_SONG_INT_PIN), changeSongISR, FALLING);

  // initialize SD card
  Serial.begin(9600);
  Serial.print("Initializing SD card...");
  pinMode(SD_SELECT, OUTPUT);
  if (!SD.begin(SD_SELECT)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  Timer3.attachInterrupt(sendData).start(1000); // multiplexing timer

  // start the first song
  changeSong();
}

void loop() {
  
  if (should_change_song) {
    changeSong();
  }

  if (should_check_note_hit) {
    // check note hits if we need to
    checkNoteHit();
  }

  if (millis() > cur_frame_time + 25) {
    // every 25 ms, send out a new frame of note data
    cur_frame_time += 25;
    updateNoteData();
    updateNoteReference();
  }

  // send audio
  if (music_file.available()) {
    music_file.read(audio_buffer, sizeof(audio_buffer));
    Audio.prepare(audio_buffer, BUFFER_SIZE, VOLUME);
    Audio.write(audio_buffer, BUFFER_SIZE);
  } else {
    song_playing = false;
  }

}