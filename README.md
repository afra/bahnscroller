bahnscroller
============

This repository contains the firmware for AfRA's Bahnscroller. The Bahnscroller is an old Fahrzielanzeige, i.e. a green 7*120 LED dot matrix display used in DB trains. I (jaseg) removed the old 80C53-based Siemens single-board computer controller board and replaced it with a little code on an arduino. The hardware is pretty simple, there are four cascaded LED boards that take 5V and have a row/column adressing based on a 3-to-8 address decoder for the rows and a 74HC595 shift register cascade for column adressing.

The firmware is just plain C code to be compiled with avr-gcc. The code is supposed to output any lines received on the arduino's serial port at 115200Bd on the display. It only displays basic ASCII, umlauts and some auxiliary unicode characters are work-in-progress.
