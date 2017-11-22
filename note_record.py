"""
note_record.py
11/21/2017
ES52 Final Project: Miniature Guitar Hero
Andrew Shackelford and Vivian Qiang
ashackelford@college.harvard.edu
vqiang@college.harvard.edu
This script records keystrokes while playing
a specified WAV file to allow the user
to record notes for use in our
Minature Guitar Hero project.
"""

import pyaudio  
import wave
import threading
import readchar
import time
import sys

class Player(threading.Thread):

    def __init__(self, song_file):
        super(Player, self).__init__()
        self.song_file = song_file

    def play_song(self, song_file):
        #define stream chunk   
        chunk = 1024  

        #open a wav format music  
        f = wave.open(song_file,"rb")  
        #instantiate PyAudio  
        p = pyaudio.PyAudio()  
        #open stream  
        stream = p.open(format = p.get_format_from_width(f.getsampwidth()),  
                        channels = f.getnchannels(),  
                        rate = f.getframerate(),  
                        output = True)  
        #read data  
        data = f.readframes(chunk)  

        #play stream  
        while data:  
            stream.write(data)  
            data = f.readframes(chunk)  

        #stop stream  
        stream.stop_stream()  
        stream.close()  

        #close PyAudio  
        p.terminate()

    def run(self):
        self.play_song(self.song_file)

def record():
    # get song_file and name
    song_file = sys.argv[1]
    song_name = song_file[:-4]

    # start playing the song
    player = Player(song_file)
    player.daemon = True
    player.start()
    start_time = time.time()

    # record each key (note) press
    notes = [[], [], [], []]
    note = ''
    while True:
        note = repr(readchar.readchar())
        note_time = time.time()
        print(note)
        if note == '\'d\'':
            notes[0].append(note_time - start_time)
        elif note == '\'f\'':
            notes[1].append(note_time - start_time)
        elif note == '\'j\'':
            notes[2].append(note_time - start_time)
        elif note == '\'k\'':
            notes[3].append(note_time - start_time)
        else:
            break

    # write the note timings out to four separate text files
    for i in range(4):
        with open(song_name + "_" + str(i) + '.txt', 'w') as f:
            for note in notes[i]:
                f.write(str(note) + '\n')

if __name__ == "__main__":
    record()

