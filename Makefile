
PROGRAMMER=arduino
PROGRAMMER_BAUDRATE=115200
UART_BAUDRATE=9600
CLOCK=16000000
MCU=atmega328p
PORT=/dev/serial/by-id/usb-Arduino__www.arduino.cc__0043_952323438333517040A1-if00

all: main.elf


main.elf: main.c font.c uart.c
	avr-gcc -Wall -fshort-enums -fno-inline-small-functions -fpack-struct -Wall -fno-strict-aliasing -funsigned-char -funsigned-bitfields -ffunction-sections -mmcu=${MCU} -DF_CPU=${CLOCK} -DUART_BAUDRATE=${UART_BAUDRATE} -std=gnu99 -Os -o main.elf -Wl,--gc-sections,--relax -I . -I../common $^
#	avr-objcopy -O ihex main.elf main.hex
	avr-size main.elf

program: main.elf
	tools/reset_arduino.py ${PORT}
	avrdude -V -c ${PROGRAMMER} -P ${PORT} -b ${PROGRAMMER_BAUDRATE} -U flash:w:main.elf -p ${MCU}

clean:
	rm -f main.elf main.hex
