
#include <stdint.h>
#include <avr/pgmspace.h>

typedef struct {
	uint8_t a, b, c, d, e;
} glyph;

const glyph font[105] PROGMEM;
