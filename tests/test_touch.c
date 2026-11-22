#include <assert.h>
#include <math.h>
#include "calibration.h"
#include "print.h"
uint16_t color, background_color;
void printArea(int a,int b,int c,int d,int e) { (void)a;(void)b;(void)c;(void)d;(void)e; }
int main(void) {
    TouchParser parser={0}; Pair point;
    const unsigned char frame[]={'X','Y',1,0x80,3,0xff,2,'\n'};
    assert(!touch_parser_feed(&parser,0xff,&point));
    for(int repeat=0;repeat<3;++repeat) {
        for(int i=0;i<8;++i) assert(touch_parser_feed(&parser,frame[i],&point)==(i==7));
        assert(point.x==384 && point.y==1023);
    }
    for(int corrupt=0;corrupt<8;++corrupt) {
        if(corrupt==3 || corrupt==5) continue; /* Legacy checksum omits low bytes. */
        parser.used=0;
        for(int i=0;i<8;++i)
            assert(!touch_parser_feed(&parser,frame[i]^(i==corrupt?0x80:0),&point));
        for(int i=0;i<8;++i) assert(touch_parser_feed(&parser,frame[i],&point)==(i==7));
    }
    for(int n=1;n<8;++n) {
        parser.used=0;
        for(int i=0;i<n;++i) touch_parser_feed(&parser,frame[i],&point);
        for(int i=0;i<8;++i) assert(touch_parser_feed(&parser,frame[i],&point)==(i==7));
    }
    Pair raw[]={{0,0},{100,0},{0,100},{100,100},{50,50}}, screen[5];
    for(int i=0;i<5;++i) screen[i]=(Pair){2*raw[i].x+0.5*raw[i].y-10,-raw[i].x+3*raw[i].y+20};
    TransformationMatrix matrix;
    assert(calculate_calibration_matrix(screen,raw,5,&matrix)==0);
    for(int i=0;i<5;++i) {
        Pair result=apply_calibration(&matrix,&raw[i]);
        assert(fabs(result.x-screen[i].x)<1e-8 && fabs(result.y-screen[i].y)<1e-8);
    }
    TransformationMatrix before=matrix;
    Pair line[]={{0,0},{1,1},{2,2}};
    assert(calculate_calibration_matrix(screen,line,3,&matrix)==-1);
    assert(matrix.a==before.a);
    assert(calculate_calibration_matrix(screen,raw,2,&matrix)==-1);
    raw[0].x=NAN; assert(calculate_calibration_matrix(screen,raw,5,&matrix)==-1);
    assert(getxy(&point)==-1);
    close_uart(); close_uart();
    return 0;
}
