/* Copyright © 2014 jaseg; Released under GPLv3 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "font.h"
#include "uart.h"
#include "main.h"

#define RBLEN   512

char EEMEM persistent_str[RBLEN];
uint16_t EEMEM persistent_len;
uint16_t eeprom_commit_count = RBLEN;

volatile uint16_t stlen = 0;
char __str[RBLEN];
char __rbuf[RBLEN];
volatile char *str = __str;
volatile char *rbuf = __rbuf;
uint16_t bindex = 0;

uint8_t lookup_unicode(uint32_t u) {
    switch (u) {
        case 0xc4: /* Ä */
            return 95;
        case 0xe4: /* ä */
            return 96;
        case 0xd6: /* Ö */
            return 97;
        case 0xf6: /* ö */
            return 98;
        case 0xdc: /* Ü */
            return 99;
        case 0xfc: /* ü */
            return 100;
        case 0xdf: /* ß */
            return 101;
        case 0x2665ULL: /* ♥ */
            return 102;
        case 0x20acULL: /* € */
            return 103;
        case 0x2605ULL: /* ★ */
            return 104;
        default:
            return 0;
    }
}

uint8_t hex_stridx(uint8_t nibble) {
    if (nibble < 10)
        return '0' + nibble - 0x20;
    return 'a' + nibble - 0xa - 0x20;
}

void scroll_hex_word(uint32_t i) {
    rbuf[bindex++] = ' ' - 0x20;
    rbuf[bindex++] = '<' - 0x20;
    rbuf[bindex++] = hex_stridx((i&0xf0000000)>>28);
    rbuf[bindex++] = hex_stridx((i&0x0f000000)>>24);
    rbuf[bindex++] = hex_stridx((i&0x00f00000)>>20);
    rbuf[bindex++] = hex_stridx((i&0x000f0000)>>16);
    rbuf[bindex++] = hex_stridx((i&0x0000f000)>>12);
    rbuf[bindex++] = hex_stridx((i&0x00000f00)>> 8);
    rbuf[bindex++] = hex_stridx((i&0x000000f0)>> 4);
    rbuf[bindex++] = hex_stridx((i&0x0000000f)>> 0);
    rbuf[bindex++] = '>' - 0x20;
    rbuf[bindex++] = ' ' - 0x20;
}

void scroll_hex_word_now(uint32_t i) {
    str[stlen++] = ' ' - 0x20;
    str[stlen++] = '<' - 0x20;
    str[stlen++] = hex_stridx((i&0xf0000000)>>28);
    str[stlen++] = hex_stridx((i&0x0f000000)>>24);
    str[stlen++] = hex_stridx((i&0x00f00000)>>20);
    str[stlen++] = hex_stridx((i&0x000f0000)>>16);
    str[stlen++] = hex_stridx((i&0x0000f000)>>12);
    str[stlen++] = hex_stridx((i&0x00000f00)>> 8);
    str[stlen++] = hex_stridx((i&0x000000f0)>> 4);
    str[stlen++] = hex_stridx((i&0x0000000f)>> 0);
    str[stlen++] = '>' - 0x20;
    str[stlen++] = ' ' - 0x20;
}

void uart_handle(char c){
    static uint32_t utf8_cp;
    static uint8_t utf8_cont;
    if(c == '\0' || c == '\n' || c == '\r'){
        stlen = bindex;
        for(uint16_t i=bindex; i<RBLEN; i++)
            rbuf[i] = 0;
        eeprom_commit_count = 0;
        volatile char *tmp = str;
        str = rbuf;
        rbuf = tmp;
        bindex = 0;
    } else if(bindex < RBLEN){
        uart_putc(c);
        if(c&0x80) {
            if(c&0x40) {
                if(c&0x20) {
                    if(c&0x10) {
                        if(c&0x08) {
                            /* invalid char */
                            utf8_cont = 0;
                        } else { /* 4-byte sequence */
                            utf8_cont = 3;
                            utf8_cp = (uint32_t)(c&0x1f) << 18;
                        }
                    } else { /* 3-byte sequence */
                        utf8_cont = 2;
                        utf8_cp = (uint32_t)(c&0x0f) << 12;
                    }
                } else { /* 2-byte sequence */
                    utf8_cont = 1;
                    utf8_cp = (uint32_t)(c&0x07) << 6;
                }
            }else{ /* Continuation byte */
                c &= 0x3f;
                if (utf8_cont) {
                    if (utf8_cont == 3)
                        utf8_cp |= (uint32_t)c << 12;
                    else if (utf8_cont == 2)
                        utf8_cp |= (uint32_t)c << 6;
                    else {
                        rbuf[bindex++] = lookup_unicode(utf8_cp | c);
                    }
                    utf8_cont -= 1;
                }
            }
        }else {
            utf8_cont = 0;
            if(c > 0x7e /* '~' */ || c < 0x20 /* ' ' (space) */)
                c = 0;
            else
                c -= 0x20;
            rbuf[bindex++] = c;
        }
    }
}

int main(void) {
    wdt_enable(WDTO_250MS);

#define PIN_WRITE(__PORT, __BIT, __VAL) {__PORT = (__PORT & ~(1<<__BIT)) | (__VAL ? (1<<__BIT) : 0);}

#define INV_MASTER_RESET(__VAL)     PIN_WRITE(PORTD, 2, __VAL)
#define SHIFT_CLOCK(__VAL)          PIN_WRITE(PORTD, 3, __VAL)
#define INV_OUTPUT_ENABLE(__VAL)    PIN_WRITE(PORTD, 4, __VAL)
#define STROBE_CLOCK(__VAL)         PIN_WRITE(PORTD, 5, __VAL)
#define ROW_ADDRESS_BIT2(__VAL)     PIN_WRITE(PORTD, 6, __VAL)
#define ROW_ADDRESS_BIT1(__VAL)     PIN_WRITE(PORTD, 7, __VAL)
#define ROW_ADDRESS_BIT0(__VAL)     PIN_WRITE(PORTB, 0, __VAL)
#define DATA_OUT(__VAL)             PIN_WRITE(PORTB, 1, __VAL)

#define CLOCK_SLEEP() _delay_us(1)

    /* Display IOs */
    DDRD |= 0xFC;
    DDRB |= 0x03;
    INV_OUTPUT_ENABLE(1); /* clear display for now */

    uart_init(UART_BAUD_SELECT(UART_BAUDRATE, F_CPU));

    stlen = eeprom_read_word(&persistent_len);
    eeprom_read_block((void *)str, &persistent_str, stlen);

    INV_MASTER_RESET(0);
    SHIFT_CLOCK(0);
    INV_OUTPUT_ENABLE(1);
    STROBE_CLOCK(0);
    ROW_ADDRESS_BIT0(0);
    ROW_ADDRESS_BIT1(0);
    ROW_ADDRESS_BIT2(0);

#define MODULE_COUNT    4
#define ROW_WIDTH       30
#define ROW_COUNT       7
#define FB_WIDTH        (MODULE_COUNT*ROW_WIDTH)
#define BOTTOM_LINE_HACK 119
    uint8_t frame_buffer[FB_WIDTH]; /* Addressed row first */

    uint8_t cycle = 0;
    int16_t offset = -FB_WIDTH;
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
        wdt_reset();
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
                if (x == BOTTOM_LINE_HACK)
                    /* HACK HACK HACK to make the line at the bottom be not brighter than the others */
                    INV_OUTPUT_ENABLE(1);
                    wdt_reset();
                if(i == 5){
                    frame_buffer[x] = 0;
                    i = 0;
                    j++;
                }else{
                    uint8_t k = 0;
                    if(j >= 0 && j < RBLEN)
                        k = str[j];
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

        if (eeprom_is_ready() && eeprom_commit_count < stlen) {
            INV_OUTPUT_ENABLE(1);

            if (eeprom_commit_count == 0) {
                /* zero out persistent_len before commencing write to avoid undefined state on reset */
                eeprom_write_word(&persistent_len, 0);
                eeprom_busy_wait();
            }

            EEAR = ((uint16_t)&persistent_str) + eeprom_commit_count;
            EEDR = str[eeprom_commit_count];
            EECR |= (1<<EEMPE);
            EECR |= (1<<EEPE);
            eeprom_commit_count++;

            if (eeprom_commit_count == stlen)
                eeprom_write_word(&persistent_len, stlen);

            INV_OUTPUT_ENABLE(0);
        }
    }
}
