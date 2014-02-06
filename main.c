/* Copyright © 2014 jaseg; Released under GPLv3
 */

#include "inc/lm4f120h5qr.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/udma.h"
#include "driverlib/ssi.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include <string.h>
#include <stdint.h>

#ifdef DEBUG
#define DEBUG_PRINT UARTprintf
#else
#define DEBUG_PRINT while(0) ((int (*)(char *, ...))0)
#endif

int main(void) {
	MAP_FPULazyStackingEnable();

	/* Set clock to PLL at 50MHz */
	MAP_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	/* Configure UART0 pins */
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
	MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
	MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	/* Configure display IOs */
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	MAP_GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
	MAP_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

	/* Enable the GPIO pins for the LED (PF2 & PF3). */
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

	UARTStdioInit(0);
	UARTprintf("Booted.\n");

#define INV_MASTER_RESET(__VAL)		MAP_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, (__VAL ? GPIO_PIN_0 : 0))
#define SHIFT_CLOCK(__VAL)			MAP_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, (__VAL ? GPIO_PIN_1 : 0))
#define INV_OUTPUT_ENABLE(__VAL)	MAP_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, (__VAL ? GPIO_PIN_2 : 0))
#define STROBE_CLOCK(__VAL)			MAP_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_3, (__VAL ? GPIO_PIN_3 : 0))
#define ROW_ADDRESS_BIT0(__VAL)		MAP_GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, (__VAL ? GPIO_PIN_4 : 0))
#define ROW_ADDRESS_BIT1(__VAL)		MAP_GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5, (__VAL ? GPIO_PIN_5 : 0))
#define ROW_ADDRESS_BIT2(__VAL)		MAP_GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, (__VAL ? GPIO_PIN_6 : 0))
#define DATA_OUT(__VAL)				MAP_GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, (__VAL ? GPIO_PIN_7 : 0))
#define LEDR(__VAL)					MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, (__VAL ? GPIO_PIN_1 : 0))
#define LEDG(__VAL)					MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, (__VAL ? GPIO_PIN_2 : 0))
#define LEDB(__VAL)					MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, (__VAL ? GPIO_PIN_3 : 0))

#define CLOCK_SLEEP() SysCtlDelay(100)

	LEDR(0);
	LEDG(0);
	LEDB(0);

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

	uint8_t state = 0;
	uint8_t row = 0;
	while(1){

		/* Reset shift registers for good measure */
		INV_MASTER_RESET(0);
		CLOCK_SLEEP();
		INV_MASTER_RESET(1);
		CLOCK_SLEEP();

		state = !state;
		LEDR(state);

		/* shift out row data */
		for(uint8_t module=0; module<MODULE_COUNT; module++){
			//for(uint32_t row_data = 0x40000000 | frame_buffer[row*MODULE_COUNT+module]; row_data; row_data<<=1){
			for(uint32_t row_data = 0x4AAAAAAA; row_data; row_data>>=1){
				DATA_OUT(row_data&1);
				CLOCK_SLEEP();
				SHIFT_CLOCK(1);
				CLOCK_SLEEP();
				SHIFT_CLOCK(0);
			}
		}

		CLOCK_SLEEP();
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
		CLOCK_SLEEP();
		row = (row+1)&7;
	}
}
