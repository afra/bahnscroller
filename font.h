
#include <stdint.h>

typedef struct {
	uint8_t a, b, c, d, e;
} glyph;

const glyph font[96] PROGMEM;
