#include <assert.h>
#include <stdlib.h>
#include "bmp.h"
#include "print.h"
extern unsigned char mock_bytes[];
extern unsigned int mock_count;
extern uint16_t screen_buffer[Y_SIZE][X_SIZE];

static unsigned coordinate(unsigned index) {
    return (unsigned)mock_bytes[index + 3] << 8 | mock_bytes[index + 7];
}
int main(void) {
    const char *names[] = {"on.bmp", "off.bmp"};
    const char *paths[] = {"images/on.bmp", "images/off.bmp"};
    for (unsigned i = 0; i < 2; ++i) {
        FILE *file = fopen(paths[i], "rb");
        assert(file);
        BmpImage image = {0};
        assert(bmp_read(file, Y_SIZE, X_SIZE, &image) == 0);
        fclose(file);
        unsigned x0 = (X_SIZE - image.height) / 2;
        unsigned y0 = (Y_SIZE - image.width) / 2;
        mock_count = 0;
        printIMG(names[i]);
        assert(coordinate(0) == x0);
        assert(coordinate(8) == x0 + image.height - 1);
        assert(coordinate(16) == y0);
        assert(coordinate(24) == y0 + image.width - 1);
        assert(mock_count == 35 + 2u * image.width * image.height);
        for (int y = 0; y < image.width; ++y) {
            for (int x = 0; x < image.height; ++x) {
                uint16_t expected = image.pixels[x * image.width + y];
                unsigned index = 35 + 2 * (y * image.height + x);
                assert(screen_buffer[y0 + y][x0 + x] == expected);
                assert(((unsigned)mock_bytes[index] << 8 | mock_bytes[index + 1]) == expected);
            }
        }
        free(image.pixels);
    }
    return 0;
}
