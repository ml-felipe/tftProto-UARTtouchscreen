#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "include/calibration.h"
#include "include/print.h"

// Calibration targets in display pixels.
static const Pair calibration_screen_points[NUM_CALIBRATION_POINTS] = {
    {0.9 * X_SIZE, 0.1 * Y_SIZE},  // top-left
    {0.9 * X_SIZE, 0.9 * Y_SIZE},  // top-right
    {0.5 * X_SIZE, 0.5 * Y_SIZE},  // center
    {0.1 * X_SIZE, 0.1 * Y_SIZE},  // bottom-left
    {0.1 * X_SIZE, 0.9 * Y_SIZE}   // bottom-right
};

TransformationMatrix cal_matrix;
int calibrated;

int calibrate(void) {
    Pair raw[NUM_CALIBRATION_POINTS];
    TransformationMatrix candidate;
    if (collect_calibration_data(raw) != 0) return -1;
    if (calculate_calibration_matrix(calibration_screen_points, raw,
                                     NUM_CALIBRATION_POINTS, &candidate) != 0) {
        fprintf(stderr, "Calibration failed: use distinct, non-collinear points\n");
        return -1;
    }
    cal_matrix = candidate;
    calibrated = 1;
    return 0;
}

int collect_calibration_data(Pair raw_points[]) {
    calibrating = 1;
    for (int i = 0; i < NUM_CALIBRATION_POINTS;) {
        Pair target = calibration_screen_points[i];
        printArea(target.x - 1, target.y - 1, target.x + 1, target.y + 1, 1);
        Pair pos;
        int status = getxy(&pos);
        uint16_t saved_color = color;
        color = background_color;
        printArea(target.x - 1, target.y - 1, target.x + 1, target.y + 1, 1);
        color = saved_color;
        if (status != 0) { calibrating = 0; return -1; }
        printf("(%.0f, %.0f)\nPress c to continue, k to repeat\n", pos.x, pos.y);
        char c;
        if (scanf(" %c", &c) != 1) { calibrating = 0; return -1; }
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
        if (c == 'c') raw_points[i++] = pos;
    }
    calibrating = 0;
    return 0;
}

int calculate_calibration_matrix(const Pair screen_points[], const Pair raw_points[], int num_points, TransformationMatrix *output) {
    // We're solving the system: A * params = B
    // For affine transformation: x' = a*x + b*y + c
    //                           y' = d*x + e*y + f
    
    if (!screen_points || !raw_points || !output || num_points < 3) return -1;
    for (int i = 0; i < num_points; ++i) {
        if (!isfinite(raw_points[i].x) || !isfinite(raw_points[i].y) ||
            !isfinite(screen_points[i].x) || !isfinite(screen_points[i].y) ||
            raw_points[i].x < 0 || raw_points[i].x > 1023 ||
            raw_points[i].y < 0 || raw_points[i].y > 1023) return -1;
    }

    // Solve using least squares (simplified approach)
    TransformationMatrix matrix;
    
    // Solve for x parameters (a, b, c)
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0, sum_y2 = 0;
    double sum_sx = 0, sum_sx_x = 0, sum_sx_y = 0;
    
    for(int i = 0; i < num_points; i++) {
        double x = raw_points[i].x;
        double y = raw_points[i].y;
        double sx = screen_points[i].x;
        
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
        sum_y2 += y * y;
        sum_sx += sx;
        sum_sx_x += sx * x;
        sum_sx_y += sx * y;
    }
    
    // sum(x^2)*a + sum(xy)*b + sum(x)*c = sum(sx*x)
    // sum(xy)*a + sum(y^2)*b + sum(y)*c = sum(sx*y)
    // sum(x)*a + sum(y)*b + n*c = sum(sx)
    
    double det = sum_x2*(sum_y2*num_points - sum_y*sum_y) - 
                 sum_xy*(sum_xy*num_points - sum_y*sum_x) + 
                 sum_x*(sum_xy*sum_y - sum_y2*sum_x);
    
    if(!isfinite(det) || fabs(det) <= 1e-12 * fmax(1.0, sum_x2 * sum_y2 * num_points)) {
        printf("Error: Calibration points are colinear or poorly conditioned\n");
        return -1;
    }
    
    matrix.a = (sum_sx_x*(sum_y2*num_points - sum_y*sum_y) - 
                sum_xy*(sum_sx_y*num_points - sum_sx*sum_y) + 
                sum_x*(sum_sx_y*sum_y - sum_y2*sum_sx)) / det;
    
    matrix.b = (sum_x2*(sum_sx_y*num_points - sum_sx*sum_y) - 
               sum_sx_x*(sum_xy*num_points - sum_x*sum_y) + 
               sum_x*(sum_xy*sum_sx - sum_sx_y*sum_x)) / det;
    
    matrix.c = (sum_x2*(sum_y2*sum_sx - sum_sx_y*sum_y) - 
                sum_xy*(sum_xy*sum_sx - sum_sx_y*sum_x) + 
                sum_sx_x*(sum_xy*sum_y - sum_y2*sum_x)) / det;
    
    // Now solve for y parameters (d, e, f)
    double sum_sy = 0, sum_sy_x = 0, sum_sy_y = 0;
    
    for(int i = 0; i < num_points; i++) {
        double x = raw_points[i].x;
        double y = raw_points[i].y;
        double sy = screen_points[i].y;
        
        sum_sy += sy;
        sum_sy_x += sy * x;
        sum_sy_y += sy * y;
    }
    
    matrix.d = (sum_sy_x*(sum_y2*num_points - sum_y*sum_y) - 
                sum_xy*(sum_sy_y*num_points - sum_sy*sum_y) + 
                sum_x*(sum_sy_y*sum_y - sum_y2*sum_sy)) / det;
    
    matrix.e = (sum_x2*(sum_sy_y*num_points - sum_sy*sum_y) - 
               sum_sy_x*(sum_xy*num_points - sum_x*sum_y) + 
               sum_x*(sum_xy*sum_sy - sum_sy_y*sum_x)) / det;
    
    matrix.f = (sum_x2*(sum_y2*sum_sy - sum_sy_y*sum_y) - 
                sum_xy*(sum_xy*sum_sy - sum_sy_y*sum_x) + 
                sum_sy_x*(sum_xy*sum_y - sum_y2*sum_x)) / det;
    
    if (!isfinite(matrix.a) || !isfinite(matrix.b) || !isfinite(matrix.c) ||
        !isfinite(matrix.d) || !isfinite(matrix.e) || !isfinite(matrix.f)) return -1;
    *output = matrix;
    return 0;
}

Pair apply_calibration(const TransformationMatrix* matrix, const Pair* raw_point) {
    Pair calibrated;
    calibrated.x = matrix->a * raw_point->x + matrix->b * raw_point->y + matrix->c;
    calibrated.y = matrix->d * raw_point->x + matrix->e * raw_point->y + matrix->f;
    return calibrated;
}

void print_matrix(const TransformationMatrix* matrix) {
    printf("[%.6f, %.6f, %.6f]\n", matrix->a, matrix->b, matrix->c);
    printf("[%.6f, %.6f, %.6f]\n", matrix->d, matrix->e, matrix->f);
    printf("[%.6f, %.6f, %.6f]\n", 0.0, 0.0, 1.0);
}