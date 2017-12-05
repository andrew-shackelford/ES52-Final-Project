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

// SD libraries
#include <SD.h>
#include <SPI.h>
#include <Audio.h>
#include <DueTimer.h>
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
#define ON_OFF_INT_PIN 26

// REF PINS
#define NOTE_REF_OUT 2
#define NOTE_CHECK_IN 47

// VOLTAGE REFERENCES
const int references[4] = {144, 97, 69, 34};

// global variables for keeping track of songs and notes
const byte NUM_SONGS = 1;
const int BUFFER_SIZE = 1024; // number of audio samples to read
const int VOLUME = 1024;

int score = 0;
byte song_num = 0;
bool note_checked = false;
bool should_check_note_hit = false;

long cur_frame_time = 0;
bool song_playing = true;
int column = 0;

short audio_buffer[BUFFER_SIZE];
bool notes[4][8];
File noteFile;
File musicFile;

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
    score = 0;
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
  song_num += 1;
  if (song_num > NUM_SONGS) {
    song_num = 1;
  }
  score = 0;
  loadNoteData();

  // start playing next song
  char str[8];
  sprintf(str, "%d", song_num);
  strcat(str, ".wav");

  Audio.begin(44100, 100);
  musicFile = SD.open(str);
  cur_frame_time = millis();

  updateNoteData();

  // update times
}

/* note data loading/updating */

void loadNoteData() {
  char str[8];
  sprintf(str, "%d", song_num);
  strcat(str, ".txt");
  noteFile = SD.open(str);
}

void updateNoteData() {
  for (byte i = 0; i < 4; i++) {
    byteToArr(noteFile.read(), notes[i]);
  }
  /*
  if (notes[0][7] || notes[1][7] || notes[2][7] || notes[3][7]) {
    should_check_note_hit = true;
    note_checked = false;
  } else {
    should_check_note_hit = false;
  }
  */
}

/* note data sending */

/**************************************************
 * sends out note data to 2 8-bit shift registers *
 * for a 4x8 multiplexed LED array                *
 **************************************************/
void sendNoteData() {
  if (column > 3) {
    column = 0;
  }
  for (byte r = 0; r < 4; r++) {
    if (r == column) {
      setPinHigh(NOTE_SDO);
    } else {
      setPinLow(NOTE_SDO);
    }
    sendClock(NOTE_CLK);
  }

  // send column data
  for (byte y = 0; y < 8; y++) {
    if (notes[3-column][7-y]) {
      setPinHigh(NOTE_SDO);
    } else {
      setPinLow(NOTE_SDO);
    }
    sendClock(NOTE_CLK);
  }
  sendClock(NOTE_LOAD);

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
  column++;
}

/* score data sending */

/**********************************************
 * send out score data to the 3-digit display *
 **********************************************/

void sendDigit(int n, int d) {
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
  setPinLow(SCORE_SDO);
  for (int i = 0; i < 7; i++) {
    sendClock(SCORE_CLK);
  }
  setPinHigh(SCORE_SDO);
  sendClock(SCORE_CLK);
  bool b[8];
  //numberToSevenSegment(n, b);
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
  int outRef = 0;
  for (int i = 0; i < 4; i++) {
    if (notes[i][7]) {
      outRef += references[i];
    }
  }
  analogWrite(NOTE_REF_OUT, outRef);
}

/*****************************************
 * compare combination of user input and *
 * reference output to see if user       *
 * hit or missed note                    *
 *****************************************/
void checkNoteHit() {
  // if we should check a note hit and we have not checked it before,
  // check it and increment the score if we hit it
  if (should_check_note_hit) {
    if (!note_checked) {
      if (!digitalRead(NOTE_CHECK_IN)) { // note check is valid
        score += 1;
        Serial.println(score);
      }
    }
    note_checked = true;
  } else {
    // otherwise, check to see if we pressed a note
    // when there was no valid note to press,
    // and if we did decrement the score
    if (digitalRead(NOTE_CHECK_IN)) { // note check is not valid
      score -= 1;
      Serial.println(score);
    }
  }
}


void setup() {
  // set our output pins
  pinMode(SD_SELECT, OUTPUT);
  pinMode(NOTE_SDO, OUTPUT);
  pinMode(NOTE_CLK, OUTPUT);
  pinMode(NOTE_LOAD, OUTPUT);

  pinMode(NOTE_CHECK_IN, INPUT);
  pinMode(NOTE_REF_OUT, OUTPUT);

  // attach interrupts to on/off and change song inputs
  //attachInterrupt(digitalPinToInterrupt(ON_OFF_INT_PIN), turnOnOffISR, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(CHANGE_SONG_INT_PIN), changeSongISR, RISING);

  // initialize SD card
  Serial.begin(9600);
  Serial.print("Initializing SD card...");
  pinMode(SD_SELECT, OUTPUT);
  if (!SD.begin(SD_SELECT)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  Timer3.attachInterrupt(sendNoteData).start(1000); // multiplexing timer

  // start the first song
  changeSong();
}

void loop() {
  /*
  if (should_turn_on_off) {
    turnOnOff();
  }
  if (should_change_song) {
    changeSong();
  }
  */
  if (song_playing) {
    if (millis() > cur_frame_time+25) {
      // every 25 ms, send out a new frame of note data
      cur_frame_time += 25;
      updateNoteData();
      updateNoteReference();
      checkNoteHit();
      //sendScoreData();
    }

    // send audio
    musicFile.read(audio_buffer, sizeof(audio_buffer));
    Audio.prepare(audio_buffer, BUFFER_SIZE, VOLUME);
    Audio.write(audio_buffer, BUFFER_SIZE);

  }
}