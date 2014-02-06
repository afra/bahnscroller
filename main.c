/* Copyright © 2014 jaseg; Released under GPLv3 */

#include <avr/io.h>
#include <util/delay.h>
#include "font.h"

int main(void) {

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
	uint32_t frame_buffer[MODULE_COUNT*ROW_COUNT]; /* Addressed row first */

	/* Test code */
	/*
	uint8_t state = 0;
	uint8_t c = 0;
	*/

	char *str = "AFRAAFRAAFRAAFRAAFRAAFRA";
	/* Render text to frame buffer */
	uint8_t offset = 0;
	for(char c=str; c && offset<MODULE_COUNT*ROW_WIDTH; c++){
		uint8_t a = pgm_read_byte(font+*c);
		uint8_t b = pgm_read_byte(font+*c+1);
		uint8_t c = pgm_read_byte(font+*c+2);
		uint8_t d = pgm_read_byte(font+*c+3);
		uint8_t e = pgm_read_byte(font+*c+4);
		for(uint8_t y=0; y<7; y++){
			frame_buffer[MODULE_COUNT*ROW_WIDTH
		}
		offset += 6;
	}

	uint8_t row = 1;
	while(1){

		/* Reset shift registers for good measure */
		INV_MASTER_RESET(0);
		CLOCK_SLEEP();
		INV_MASTER_RESET(1);
		CLOCK_SLEEP();

		/* Test code */
		/*
		c++;
		if(c > 40){
			c=0;
			state++;
			if(state >= 37)
				state = 0;
		}
		*/

		/* shift out row data */
		for(uint8_t module=0; module<MODULE_COUNT; module++){
			uint32_t row_data = frame_buffer[(row-1)*MODULE_COUNT+module];
			/* Test code */
			/*
			if(state < 30)
				row_data = (uint32_t)1<<state;
			else
				row_data = 0x3FFFFFFF * (row == state-29);
			*/

			for(row_data |= 0x80000000; row_data; row_data>>=1){
				DATA_OUT(row_data&1);
				CLOCK_SLEEP();
				SHIFT_CLOCK(1);
				CLOCK_SLEEP();
				SHIFT_CLOCK(0);
			}
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
		if(row == 8)
			row = 1;
	}
}
