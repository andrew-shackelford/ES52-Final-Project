"""
note_convert.py
11/21/2017
ES52 Final Project: Miniature Guitar Hero
Andrew Shackelford and Vivian Qiang
ashackelford@college.harvard.edu
vqiang@college.harvard.edu
This script converts 4 lists of note timings
into LED frames for the Arduino to read.
"""

import sys
NOTE_SEP_TIME = 0.15 # number of seconds between note rows

def convert():
    note_list = [[], [], [], []]

    # open each list of note timings
    for i in range(4):
        with open(sys.argv[1] + "_" + str(i) + ".txt", 'r') as f:
            for line in f:
                time = float(line)
                note_list[i].append(time)
            if len(note_list[i]) == 0:
                note_list[i].append(-100.)

    # write out the LED frames for each set of notes as 4 sequential bytes,
    # with 1 byte per column and 25 ms per frame (40 Hz)
    time = 0.
    end = max(note_list[0][-1], note_list[1][-1], note_list[2][-1], note_list[3][-1]) + 2.
    with open(sys.argv[1] + "_led.txt", 'w') as f:
        while time < end:
            for i in range(4):
                out = 0
                for note in note_list[i]:
                    if time - 1*NOTE_SEP_TIME < note <= time:
                        out += 128
                    if time < note <= time + 1*NOTE_SEP_TIME:
                        out += 64
                    if time + 1*NOTE_SEP_TIME < note <= time + 2*NOTE_SEP_TIME:
                        out += 32
                    if time + 2*NOTE_SEP_TIME < note <= time + 3*NOTE_SEP_TIME:
                        out += 16
                    if time + 3*NOTE_SEP_TIME < note <= time + 4*NOTE_SEP_TIME:
                        out += 8
                    if time + 4*NOTE_SEP_TIME < note <= time + 5*NOTE_SEP_TIME:
                        out += 4
                    if time + 5*NOTE_SEP_TIME < note <= time + 6*NOTE_SEP_TIME:
                        out += 2
                    if time + 6*NOTE_SEP_TIME < note <= time + 7*NOTE_SEP_TIME:
                        out += 1
                try:
                    f.write(chr(out))
                except:
                    print(out)
            time += 0.025

def main():
    convert()

if __name__ == "__main__":
    main()