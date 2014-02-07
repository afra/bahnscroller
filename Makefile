
PROGRAMMER=arduino
PROGRAMMER_BAUDRATE=115200
CLOCK=16000000
MCU=atmega328p
PORT=/dev/ttyACM0

all: objects


objects: main.c font.c
	avr-gcc -Wall -fshort-enums -fno-inline-small-functions -fpack-struct -Wall -fno-strict-aliasing -funsigned-char -funsigned-bitfields -ffunction-sections -mmcu=${MCU} -DF_CPU=${CLOCK} -std=gnu99 -Os -o main.elf -Wl,--gc-sections,--relax -I . -I../common $^
	avr-objcopy -O ihex main.elf main.hex
	avr-size main.elf

program: objects
	tools/reset_arduino.py ${PORT}
	avrdude -V -c ${PROGRAMMER} -P ${PORT} -b ${PROGRAMMER_BAUDRATE} -U flash:w:main.hex -p ${MCU}

clean:
	rm *.elf main.hex || true
