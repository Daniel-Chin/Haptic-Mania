from os import path
from typing import List, Tuple
from time import time, sleep

import serial

from osu_parser.beatmapparser import BeatmapParser

PORT = 'COM10'
SONG_OFFSET = 900

K = 4
BEATMAP_DIR = path.abspath(
    './626677 O2i3 - Capitalism Cannon', 
)
MAP_PATH = path.join(
    BEATMAP_DIR, 
    'O2i3 - Capitalism Cannon (Ilham) [Lv.7 Yellow].osu', 
)

CIRCLE = 'circle'
MANIA_SLIDER = 'mania_slider'
TIME_LEN = 3

class HitObject:
    def __init__(self, o, /):
        if o['object_name'] == CIRCLE:
            self.object_type = CIRCLE
        elif o['object_name'] == MANIA_SLIDER:
            self.object_type = MANIA_SLIDER
        else:
            raise ValueError(o['object_name'])
        self.start_time = o['startTime']
        if self.object_type is MANIA_SLIDER:
            self.end_time = o['end_time']

        x, y = o['position']
        assert K == 4
        assert y == 192
        position = (x - 64) / 128
        self.position = round(position)
        assert position - self.position == 0.0

def flatten(hObs: List[HitObject], click_duration=80):
    results: List[Tuple[int, int]] = []

    # Hold "`"
    results.append((0, 0))
    results.append((500, 0))

    for hOb in hObs:
        results.append((
            hOb.start_time + SONG_OFFSET, 
            hOb.position, 
        ))
        if hOb.object_type is MANIA_SLIDER:
            release_t = hOb.end_time
        else:
            release_t = hOb.start_time + click_duration
        results.append((
            release_t + SONG_OFFSET, 
            hOb.position, 
        ))
    results.sort(key = lambda x : x[0])
    return results

def preview(hObs: List[HitObject]):
    CLICK_HOLD_TIME = 50

    start = time()
    song = iter(hObs)
    hOb = next(song)
    time_to_release = [0] * K
    t = 0
    def display():
        print(
            *[' ' if t >= x else '#' for x in time_to_release], 
            flush=True, 
        )
    while True:
        t = round((time() - start) * 1000)
        if t >= hOb.start_time:
            if hOb.object_type is CIRCLE:
                ttr = t + CLICK_HOLD_TIME
            else:
                ttr = hOb.end_time
            time_to_release[hOb.position] = ttr
            try:
                hOb = next(song)
            except StopIteration:
                break
        display()
        sleep(0.02)

def main():
    parser = BeatmapParser()
    parser.parseFile(MAP_PATH)
    map = parser.build_beatmap()
    hObs = [HitObject(x) for x in map['hitObjects']]
    # preview(hObs)
    # return
    toggles = flatten(hObs)

    with serial.Serial(PORT, 9600, timeout=1) as s:
        s.timeout = None
        print('Serial opened:', s.name)
        handshake(s)
        while True:
            broken = False
            try:
                song = iter(toggles)
                while True:
                    recved = s.read(1)
                    if recved == b'r':
                        pass
                    elif recved == b'd':
                        msg = s.read(16)
                        print('Arduino msg: ', msg.decode())
                    else:
                        dumpSerial(s, recved)
                        raise ValueError
                    try:
                        toggle_t, toggle_pos = next(song)
                    except StopIteration:
                        s.write(b'e')
                        break
                    else:
                        print(toggle_t)
                    s.write(b'n')
                    s.write(encodeTime(toggle_t))
                    s.write(bytes([b'0123'[toggle_pos]]))
            except KeyboardInterrupt:
                broken = True
            if broken:
                try:
                    input('Press Enter to start, ctrl+Z to quit...')
                except (KeyboardInterrupt, EOFError):
                    break
                sleep(1)
                s.reset_input_buffer()
                s.write(b'b')
        print('bye')

def handshake(s: serial.Serial):
    print('Trying to shake hands...')
    s.write(b'hi arduino')
    expecting = [*'hi python']
    silence = 0
    while expecting:
        if s.in_waiting:
            recved = s.read(1)
            expected = expecting.pop(0).encode()
            if recved != expected:
                raise ValueError((recved, expected))
        else:
            silence += 1
            sleep(.1)
            if silence > 10:
                handshake(s)
                return
    print('handshook')

def encodeTime(t):
    buf = []
    for _ in range(TIME_LEN):
        x = t % 126
        buf.append(x + 1)
        t //= 126
    return bytes(buf)

def decodeTime(x: bytes):
    acc = 0
    for i, char in enumerate(x):
        acc += char * 126 ** (i - 1)
    return acc

def dumpSerial(s: serial.Serial, head):
    s.timeout = 1
    x = s.read(9999)
    print('Serial dump:')
    print(head + x)

main()
