/* Copyright © 2014 jaseg; Released under GPLv3 */

#include <avr/io.h>
#include <util/delay.h>

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
#define ROW_WIDTH		32
#define ROW_COUNT		8
	uint32_t frame_buffer[MODULE_COUNT*ROW_COUNT]; /* Addressed row first */

	uint8_t row = 0;
	while(1){

		/* Reset shift registers for good measure */
		INV_MASTER_RESET(0);
		CLOCK_SLEEP();
		INV_MASTER_RESET(1);
		CLOCK_SLEEP();

		/* shift out row data */
		for(uint8_t module=0; module<MODULE_COUNT; module++){
			//for(uint32_t row_data = 0x40000000 | frame_buffer[row*MODULE_COUNT+module]; row_data; row_data<<=1){
			for(uint32_t row_data = ((row&1) ? 0x6AAAAAAA : 0x55555555); row_data; row_data>>=1){
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
		row = (row+1)&7;
	}
}
