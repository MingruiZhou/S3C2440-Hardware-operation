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

    	uart0_init();   // ������115200��8N1(8������λ����У��λ��1��ֹͣλ)
    	Lcd_Port_Init();                     // ����LCD����
    	Tft_Lcd_Init(); // ��ʼ��LCD������
    	Lcd_PowerEnable(0, 1);               // ����LCD_PWREN��Ч�������ڴ�LCD�ĵ�Դ
    	Lcd_EnvidOnOff(1);                   // ʹ��LCD����������ź�

    	ClearScr(50);  // ��������ɫssssssssssssssssssssssssssssssssssssssss
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
