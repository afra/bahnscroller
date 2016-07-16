#!/usr/bin/env python3

import time
import serial
from bvggrabber.api.actualdeparture import ActualDepartureQueryApi
from bvggrabber.api.scheduleddeparture import ScheduledDepartureQueryApi, Vehicle

PORT = '/dev/serial/by-id/usb-Arduino__www.arduino.cc__0043_952323438333517040A1-if00'

def departures():
    query = ActualDepartureQueryApi('Herzbergstr./Siegfriedstr. (Berlin)')
    res = query.call()
    return [ '{:%H:%M} {} {}'.format(departure.when, departure.line.split()[-1], departure.end)
            for start, departures in res.departures
            for departure in sorted(departures, key=lambda dep:dep.when) ]

ser = serial.Serial(PORT, 115200, timeout=0)
time.sleep(3)
while True:
    line = ' | '.join(departures())
    for a, b in (
            ('ä', 'ae'),
            ('Ä', 'AE'),
            ('ö', 'oe'),
            ('Ö', 'OE'),
            ('ü', 'ue'),
            ('Ü', 'UE'),
            ('ß', 'ss'),
            ):
        line = line.replace(a, b)
    print(line)
    ser.write((line+'\r').encode('utf8'))
    time.sleep(300)
    
