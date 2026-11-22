#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "calibration.h"
#include "print.h"
int calibrating, pressing;
uint16_t color = 1, background_color;
static int samples, draws, fail_read;
void printArea(int x1,int y1,int x2,int y2,int fill) {
    assert(x1 >= 0 && x2 < X_SIZE && y1 >= 0 && y2 < Y_SIZE && fill);
    ++draws;
}
int getxy(Pair *point) {
    if (fail_read) return -1;
    *point = (Pair){++samples, 10}; return 0;
}
int main(void) {
    FILE *input = tmpfile(); assert(input);
    fputs("k\nc\nc\nc\nc\nc\n", input); rewind(input);
    assert(dup2(fileno(input), STDIN_FILENO) >= 0);
    Pair raw[5];
    assert(collect_calibration_data(raw) == 0);
    assert(samples == 6 && draws == 12 && !calibrating && color == 1);
    assert(raw[0].x == 2 && raw[4].x == 6);
    fail_read = 1;
    assert(collect_calibration_data(raw) == -1 && !calibrating && color == 1);
    fclose(input);
    return 0;
}
