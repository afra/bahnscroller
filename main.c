/* Copyright © 2014 jaseg; Released under GPLv3 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "font.h"
#include "uart.h"
#include "main.h"

#define RBLEN	512
volatile uint16_t stlen = RBLEN;
char __str[RBLEN];
char __rbuf[RBLEN];
volatile char *str = __str;
volatile char *rbuf = __rbuf;
uint16_t bindex = 0;

void uart_handle(char c){
	uart_putc(c);
	if(bindex < RBLEN)
		rbuf[bindex++] = c;
	if(c == '\0' || c == '\n' || c == '\r'){
		uart_putc('\n');
		stlen = bindex;
		for(uint16_t i=bindex; i<RBLEN; i++)
			rbuf[i] = 0;
		volatile char *tmp = str;
		str = rbuf;
		rbuf = tmp;
		bindex = 0;
	}
}

int main(void) {

    uart_init(UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUDRATE, F_CPU));
	for(uint16_t i=0; i<RBLEN; i++){
		uint8_t j = i%36;
		if(j<10)
			str[i] = '0'+j;
		else
			str[i] = 'A'+j-10;
	}

	/* Display IOs */
	DDRD |= 0xFC;
	DDRB |= 0x03;

#define PIN_WRITE(__PORT, __BIT, __VAL) {__PORT = (__PORT & ~(1<<__BIT)) | (__VAL ? (1<<__BIT) : 0);}

#define INV_MASTER_RESET(__VAL)		PIN_WRITE(PORTD, 2, __VAL)
#define SHIFT_CLOCK(__VAL)			PIN_WRITE(PORTD, 3, __VAL)
#define INV_OUTPUT_ENABLE(__VAL)	PIN_WRITE(PORTD, 4, __VAL)
#define STROBE_CLOCK(__VAL)			PIN_WRITE(PORTD, 5, __VAL)
#define ROW_ADDRESS_BIT2(__VAL)		PIN_WRITE(PORTD, 6, __VAL)
#define ROW_ADDRESS_BIT1(__VAL)		PIN_WRITE(PORTD, 7, __VAL)
#define ROW_ADDRESS_BIT0(__VAL)		PIN_WRITE(PORTB, 0, __VAL)
#define DATA_OUT(__VAL)				PIN_WRITE(PORTB, 1, __VAL)

#define CLOCK_SLEEP() _delay_us(1)

	INV_MASTER_RESET(0);
	SHIFT_CLOCK(0);
	INV_OUTPUT_ENABLE(1);
	STROBE_CLOCK(0);
	ROW_ADDRESS_BIT0(0);
	ROW_ADDRESS_BIT1(0);
	ROW_ADDRESS_BIT2(0);

#define MODULE_COUNT	4
#define ROW_WIDTH		30
#define ROW_COUNT		7
#define FB_WIDTH		(MODULE_COUNT*ROW_WIDTH)
	uint8_t frame_buffer[FB_WIDTH]; /* Addressed row first */

	uint8_t cycle = 0;
	int16_t offset = 0;
	uint8_t row = 1;
	uint8_t row_mask = 1;

	sei();
	while(1){
		/* Reset shift registers for good measure */
		INV_MASTER_RESET(0);
		CLOCK_SLEEP();
		INV_MASTER_RESET(1);
		CLOCK_SLEEP();

		/* shift out row data */
		for(uint8_t module=0; module<MODULE_COUNT; module++){
			uint8_t m_off = module*ROW_WIDTH;
			for(uint8_t i=0; i<ROW_WIDTH; i++){
				DATA_OUT(frame_buffer[m_off+i]&row_mask);
				CLOCK_SLEEP();
				SHIFT_CLOCK(1);
				CLOCK_SLEEP();
				SHIFT_CLOCK(0);
			}
			/* Shift out two dummy bits */
			DATA_OUT(0);
			CLOCK_SLEEP();
			SHIFT_CLOCK(1);
			CLOCK_SLEEP();
			SHIFT_CLOCK(0);
			CLOCK_SLEEP();
			SHIFT_CLOCK(1);
			CLOCK_SLEEP();
			SHIFT_CLOCK(0);
		}

		INV_OUTPUT_ENABLE(1);
		CLOCK_SLEEP();
		ROW_ADDRESS_BIT0(row&1);
		ROW_ADDRESS_BIT1(row&2);
		ROW_ADDRESS_BIT2(row&4);
		CLOCK_SLEEP();
		STROBE_CLOCK(1);
		CLOCK_SLEEP();
		STROBE_CLOCK(0);
		INV_OUTPUT_ENABLE(0);
		row++;
		row_mask <<= 1;
		if(row == 8){
			row = 1;
			row_mask = 1;

			/* Render text to frame buffer */
			int8_t i = offset%6;
			if(i<0)
				i += 6;
			int16_t j;
			if(offset < 0)
				j = (offset+1)/6-1;
			else
				j = offset/6;
			for(uint8_t x=0; x<FB_WIDTH; x++){
				if(i == 5){
					frame_buffer[x] = 0;
					i = 0;
					j++;
				}else{
					uint8_t k = 0;
					if(j >= 0 && j < RBLEN){
						k = str[j] - 0x20;
						if(k > 96)
							k = 0;
					}
					frame_buffer[x] = pgm_read_byte((uint8_t*)(font+k) + i);
					i++;
				}
			}

			cycle++;
			if(cycle == 10){
				cycle=0;
				offset++;
				if(offset > (int16_t)stlen*6){
					offset = -FB_WIDTH;
				}
			}
		}
	}
}
