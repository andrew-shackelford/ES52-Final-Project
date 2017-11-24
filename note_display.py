"""
note_display.py
11/21/2017
ES52 Final Project: Miniature Guitar Hero
Andrew Shackelford and Vivian Qiang
ashackelford@college.harvard.edu
vqiang@college.harvard.edu
This script plays back a WAV file and its
corresponding file of note frames
to simulate what playback should look
like in the final product with
the Arduino and corresponding LEDs,
so that any note timing errors can be fixed
before loading onto the Arduino.
"""

import time
import sys
import wave
import threading
import pyaudio

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

def print_line(note_zero, note_one, note_two, note_three):
    if note_zero:
        str_zero = "0 "
    else:
        str_zero = "_ "
    if note_one:
        str_one = "0 "
    else:
        str_one = "_ "
    if note_two:
        str_two = "0 "
    else:
        str_two = "_ "
    if note_three:
        str_three = "0 "
    else:
        str_three = "_ "
    print(str_zero + str_one + str_two + str_three)

def clear_lines():
    for i in range(9):
        sys.stdout.write("\033[F")

def display():
    # get song and note files
    song_name = sys.argv[1]
    song_file = sys.argv[1] + ".wav"
    note_file = sys.argv[1] + "_led.txt"
    led_list = [[], [], [], []]

    # load in all the note frames
    count = 0
    with open(note_file, 'r') as f:
        while True:
            byte = f.read(1)
            if not byte:
                break
            led_line = []
            for bit in '{0:08b}'.format(ord(byte)):
                led_line.append(int(bit))
            led_list[count%4].append(led_line)
            count += 1

    # start playing the song
    player = Player(song_file)
    player.daemon = True
    index = 1
    player.start()
    cur_time = time.time()
    start_time = cur_time

    # print the first frame
    for j in range(8):
        print_line(led_list[0][0][j], led_list[1][0][j], led_list[2][0][j], led_list[3][0][j])
    print(time.time() - start_time)

    # print all subsequent frames
    while True:
        if time.time() > cur_time + 0.025:
            cur_time += 0.025
            clear_lines()
            for j in range(8):
                print_line(led_list[0][index][7-j], led_list[1][index][7-j], led_list[2][index][7-j], led_list[3][index][7-j])
            print(time.time() - start_time)
            index += 1

if __name__ == "__main__":
    display()
