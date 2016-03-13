/*
 * FILE: lcdlib.h
 * TFT LCD的测试函数接口
 */

#ifndef __LCDLIB_H__
#define __LCDLIB_H__

typedef struct Video_t {
    char    TextAttr;
    char    X;      
    char    Y;
} VideoT;
typedef VideoT* VideoPtr;
  
typedef struct Window_t {
    char    X1,Y1;      
    char    X2,Y2;
} WindowT;
typedef WindowT* WindowPtr;


void Test_Lcd_Tft_16Bit_480272(void);
void VideoInit() ;
int printf_k(char* fmt, ...);

#endif /*__LCDLIB_H__*/
