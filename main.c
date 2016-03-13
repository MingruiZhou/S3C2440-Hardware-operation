#include "serial.h"
#include "lcdlib.h"
#include "s3c24xx.h"
#include "lcddrv.h"
#include "framebuffer.h"
#include "lib/printf.h"
#include "string.h"

int main()
{
	char data[100];
	uart0_init(); 

    	uart0_init();   // 波特率115200，8N1(8个数据位，无校验位，1个停止位)
    	Lcd_Port_Init();                     // 设置LCD引脚
    	Tft_Lcd_Init(); // 初始化LCD控制器
    	Lcd_PowerEnable(0, 1);               // 设置LCD_PWREN有效，它用于打开LCD的电源
    	Lcd_EnvidOnOff(1);                   // 使能LCD控制器输出信号

    	ClearScr(50);  // 清屏，黑色ssssssssssssssssssssssssssssssssssssssss
    	VideoInit();	//setting font color and windwos size
    	printf_k("\n\rhao are you\n\r");
    	while (1) {
		memset(data, 0, sizeof(data));
		gets(data);
		puts(data);
		printf_k("%s\n\r", data);
	}
    
    return 0;
}
