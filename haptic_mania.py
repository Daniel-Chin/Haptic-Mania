from os import path
from typing import List
from time import time, sleep

from osu_parser.beatmapparser import BeatmapParser

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

class HitObject:
    def __init__(self, o, /):
        if o['object_name'] == CIRCLE:
            self.object_type = CIRCLE
        elif o['object_name'] == MANIA_SLIDER:
            self.object_type = MANIA_SLIDER
        else:
            raise ValueError(o['object_name'])
        self.startTime = o['startTime']
        if self.object_type is MANIA_SLIDER:
            self.end_time = o['end_time']

        x, y = o['position']
        assert K == 4
        assert y == 192
        position = (x - 64) / 128
        self.position = round(position)
        assert position - self.position == 0.0

def preview(hObs: List[HitObject]):
    CLICK_HOLD_TIME = 100

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
        if t >= hOb.startTime:
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
    preview(hObs)

main()
