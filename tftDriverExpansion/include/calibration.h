#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "uart_touchscreen.h"

#define NUM_CALIBRATION_POINTS 5

typedef struct {
    double a, b, c;
    double d, e, f;
    // Last row is implicitly [0, 0, 1] for 2D transformations
} TransformationMatrix;

extern TransformationMatrix cal_matrix;
extern int calibrated;

int collect_calibration_data(Pair raw_points[]);
int calculate_calibration_matrix(const Pair screen_points[], const Pair raw_points[], int num_points, TransformationMatrix *output);
Pair apply_calibration(const TransformationMatrix* matrix, const Pair* raw_point);
void print_matrix(const TransformationMatrix* matrix);
int calibrate(void);

#endif   