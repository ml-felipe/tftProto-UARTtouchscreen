#include <string.h>
#include "include/uart_touchscreen.h"
int touch_parser_feed(TouchParser *parser, unsigned char byte, Pair *point) {
    if (!parser || !point) return 0;
    if (parser->used == MSG_LENGTH) {
        memmove(parser->bytes, parser->bytes + 1, MSG_LENGTH - 1);
        --parser->used;
    }
    parser->bytes[parser->used++] = byte;
    const unsigned char *b = parser->bytes;
    if (parser->used != MSG_LENGTH || b[0] != 'X' || b[1] != 'Y' ||
        b[7] != '\n' || b[2] > 3 || b[4] > 3 || b[6] != (b[2] ^ b[4])) return 0;
    *point = (Pair){(b[2] << 8) | b[3], (b[4] << 8) | b[5]};
    parser->used = 0;
    return 1;
}
