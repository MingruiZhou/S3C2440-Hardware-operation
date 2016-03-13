/*
 * FILE: lcdlib.c
 * 实现TFT LCD的测试函数
 */

#include "lcddrv.h"
#include "lcdlib.h"
#include "framebuffer.h"
#include "serial.h"
#include "lib/printf.h"

VideoT AtCursor;
static WindowT vWindow;
int ViewPortX;
int ViewPortY;

#define LCDWIDTH   	60
#define LCDHEIGHT	33

#define MAXPUTSTRINGLEN   1024
#define FMT_LONG 44 /* enough space to hold 2^128 - 1 in decimal or octal plus \0 */
#define ALT_FORM	0x01
#define L_ADJUST	0x02
#define ZERO_PAD	0x04
#define SIGNED		0x08
#define LONG_INT	0x11
#define SHORT_INT	0x12
#define VOID_PTR	0x14
#define SIGN_MASK   (1UL << (sizeof(long)*8-1))


#define set_flag(f)		(flags |= f)
#define is_flag(f)		(flags & f)

#define va_start(v,l)	__builtin_stdarg_start((v),l)
#define va_end		__builtin_va_end
#define va_arg		__builtin_va_arg
typedef __builtin_va_list va_list;


extern  unsigned short rom8x8_bits[] ;

static const char nullString[] = "(null pointer)";
static const char ioHexArray[16] =  {'0','1','2','3','4','5','6','7',
                                     '8','9','a','b','c','d','e','f'};


/* 
 * 以480x272,16bpp的显示模式测试TFT LCD
 */
void Test_Lcd_Tft_16Bit_480272(void)
{
    Lcd_Port_Init();                     // 设置LCD引脚
    Tft_Lcd_Init(); // 初始化LCD控制器
    Lcd_PowerEnable(0, 1);               // 设置LCD_PWREN有效，它用于打开LCD的电源
    Lcd_EnvidOnOff(1);                   // 使能LCD控制器输出信号

    ClearScr(0x0);  // 清屏，黑色
    VideoInit();
	
    printf("[TFT 64K COLOR(16bpp) LCD TEST]\n");

    printf("1. Press any key to draw line\n");
    printf_k("1. Press any key to draw line\n\r");
    getc();
    DrawLine(0  , 0  , 479, 0  , 0xff0000);    // 红色
    DrawLine(0  , 0  , 0  , 271, 0x00ff00);    // 绿色
    DrawLine(479, 0  , 479, 271, 0x0000ff);    // 蓝色
    DrawLine(0  , 271, 479, 271, 0xffffff);    // 白色
    DrawLine(0  , 0  , 479, 271, 0xffff00);    // 黄色
    DrawLine(479, 0  , 0  , 271, 0x8000ff);    // 紫色
    DrawLine(240, 0  , 240, 271, 0xe6e8fa);    // 银色
    DrawLine(0  , 136, 479, 136, 0xcd7f32);    // 金色

    printf("2. Press any key to draw circles\n");
	printf_k("2. Press any key to draw circles\n\r");
    getc();
    Mire();

    printf("3. Press any key to fill the screem with one color\n");
	printf_k("3. Press any key to fill the screem with one color\n\r");
    getc();
    ClearScr(0xff0000);             // 红色

    printf("4. Press any key to fill the screem by temporary palette\n");
	printf_k("4. Press any key to fill the screem by temporary paletten\r");
    getc();
    ClearScrWithTmpPlt(0x0000ff);   // 蓝色

    printf("5. Press any key stop the testing\n");
	printf_k("5. Press any key stop the testing\r");
    getc();
    Lcd_EnvidOnOff(0);
}




void __PutChar(UINT32 cx, UINT32 cy, UINT8 c, UINT32 color)
{
	unsigned short  index=(c<<3);
	unsigned char bit_mask;
	int x,y;
	for(y=0;y<8;y++)
	{
		bit_mask=0x80;
		for(x=0;x<8;x++)
		{
			if((rom8x8_bits[index]>>8)&bit_mask)
				PutPixel(x+cx,cy,color);
			bit_mask=(bit_mask>>1);
		}
		cy++;
		index++;
	}

}

void kprintc(char pc) {
	if (pc == '\n')
    	AtCursor.X=vWindow.X2+1;
  	else if (pc != '\r' && pc != 8 && pc!= 9) {
       	if ( AtCursor.X<LCDWIDTH&&AtCursor.Y<LCDHEIGHT) {
	   		__PutChar(AtCursor.X*8, AtCursor.Y*8, pc,AtCursor.TextAttr);
         	AtCursor.X++; 
		}
    }
  	if(pc==8 && AtCursor.X>vWindow.X1+2) {
		AtCursor.X--;
        __PutChar(AtCursor.X*8, AtCursor.Y*8, 32,AtCursor.TextAttr);
    }
    if (AtCursor.X>vWindow.X2) {          
	    AtCursor.Y++;
		//ViewPortY++;
	    AtCursor.X=vWindow.X1;
  	}
  	if (AtCursor.Y > vWindow.Y2) {
    	AtCursor.Y=vWindow.Y2;
	Scroll();
  	}
} 

void ioConsolePutChar(char c) {
  kprintc(c);
}

void ioConsolePutString(const char* s) {
	int i = 0;

	if (s == NULL) s = nullString;
	while ((s[i] != '\0') && (i < MAXPUTSTRINGLEN)) {
	   /* Translate newlines into carriage returns plus newlines for
	    * correct terminal output
	    */
	   	if (s[i] == '\n') {
			ioConsolePutChar('\r');
	   	}
		ioConsolePutChar(s[i++]);
	}
}

unsigned long int str_len(register char *s)
{
  register char *t;

  t = s;
  for (;;) {
    if (!*t) return t - s; ++t;
    if (!*t) return t - s; ++t;
    if (!*t) return t - s; ++t;
    if (!*t) return t - s; ++t;
  } 
} 

void str_cpy( char* target, char* source)
{
	while ((*target++ = *source++) != '\0');
}

static void format_long(char *buf, unsigned long val, char type, int prec, char* sign, int flags)
{
	int base;
	char tmp[FMT_LONG];
	int pos=0, i;
	char *digit = "0123456789ABCDEF";
	if( is_flag(SIGNED) && (val & SIGN_MASK)) {
		*buf++ = '-';
		val = ~val + 1; /* other vay to say val*=-1 :-) */
	} else if ( is_flag(SIGNED) && sign ) {
		*buf++ = *sign;
	}
	
	switch(type) {
		case 'd':
			base = 10;
			break;
		case 'o':
			base = 8;
			break;
		case 'x':
			digit = "0123456789abcdef";
		case 'X':
			base = 16;
			break;
		default:
			str_cpy(buf, "bug in vsprintf: bad base");
			return;
	}    
	if(val==0) { tmp[0]=digit[0]; pos++; }
	else while(val) {
		tmp[pos] = digit[val%base];
		val = val / base;
		pos++;
	}
	/* in tmp ist the unsigned number; pos is lenght */
	if(prec > 0 && pos < prec ) { 
		if ( prec >= FMT_LONG ) prec = FMT_LONG-1; /* overflow protection */
		for(i=pos; i<prec; i++) *buf++ = '0'; /* pad with 0 if wanted precision is larger */
   	}
	for(pos--; pos>=0; pos--) {
		*buf++ = tmp[pos];
	}
	*buf = '\0';
}

unsigned long f_to_l(float l)
{
 unsigned long num = 0;
 while(l>=(0*0.00000000001))
 	{
 	l = l - 1;
 	num = num + 1;
 	}
 return (num-1);
}


void format_float(char *buf, double val,  int prec, char* sign, int flags)
{
	char tmp[44];
	int pos=0;
	char *digit = "0123456789";
	unsigned long tmpnum;
	double i = 1;
   
  if(val > 1){
  	    tmpnum = f_to_l(val);
  	    val = val - tmpnum;
  		while(tmpnum) {
		tmp[pos] = digit[tmpnum%10];
		tmpnum = tmpnum / 10;
		pos++;
	}  		
  	for(pos--; pos>=0; pos--) {
		*buf++ = tmp[pos];
	   }
  		goto next; 
  	}
  else {
  	*buf++ = '0';
 next: 	*buf++ ='.';  	
  	while(prec>0){
  		tmpnum = f_to_l((val/(0.1*i)));
  		//printf_k("the prec is %d,and the tmpnum is %d",prec,tmpnum);
  		if(tmpnum>=0&&tmpnum<=9)
  		*buf++ = digit[tmpnum];
  		val = val -(tmpnum*(0.1*i));
  		if(val<=0)
  		goto endprec;
  		i = i * 0.1;
  		prec--;
  			}
  		}
  endprec:
    *buf = '\0';
}


int myvsnprintf(char *buffer, unsigned long len, char *fmt, va_list ap,int type) 
{
/* first some macros */
#define mwrite(c) \
		if ( len ) { \
			*buffer++ = c; \
			len--; \
		} \
		written++ /* count even if buffer full (posix) */
/* Macros for converting digits to letters and vice versa */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')
#define get_width(ui, s) \
		ui = 0; \
		while ( is_digit(*s) ) { \
			ui = 10 * ui + to_digit(*s++); \
		} \
		--s /* while goes one to far */
	unsigned long written = 0, tlen;
	char *sign = 0;
	char numbuf[FMT_LONG];
	int flags, done, field, pfield, i;
	char base; /* d for decimal, x for hex, X for HEX, o for octal */
	/* vars for va_arg */
	char c, *s;
	long l; /* shorts will be stored in longs */
	unsigned long ul;
	double t = 0;
	

	buffer[--len] = '\0'; /* buffer has to be 0-Terminated */
	while ( *fmt ) {
		if (*fmt == '%') {
			done = 1; flags = 0; pfield = 0; field = 0; sign = 0;
			do {
				switch(*++fmt) {
					case '#':
						done = 0; set_flag(ALT_FORM);
						break;
					case '-':
						done = 0; set_flag(L_ADJUST);
						break;
					case '+':
						done = 0; sign = "+";
						break;
					case ' ':
						done = 0;
						if (! sign)
							sign = " ";
						break;
					case '0':
						done = 0; set_flag(ZERO_PAD);
						break;
					case '.':
						done = 0; 
						++fmt; /* start with next char */
						get_width(pfield, fmt);
						break;
					case '1': case '2': case '3': case '4': case '5':
					case '6': case '7': case '8': case '9':
						done = 0;
						get_width(field, fmt);
						break;
					case 'l':
						done = 0; set_flag(LONG_INT);
						break;
					case 'h':
						done = 0; set_flag(SHORT_INT);
						break;
					case 'f':
						done = 1;
						base = 'f';
						set_flag(SIGNED);
					    goto floatnumber;
					case 'd': case 'i':
						done = 1;
						base = 'd';
						set_flag(SIGNED);
						goto number;
					case 'o':
						done = 1;
						base = 'o';
						goto number;
					case 'u':
						done = 1;
						base = 'd';
						goto number;
					case 'X':
						done = 1;
						base = 'X';
						goto number;
					case 'x':
						done = 1;
						base = 'x';
						goto number;
					case 'c':
						done = 1;
						c = (unsigned char)va_arg(ap, int);
						mwrite(c);
						break;
					case 's':
						done = 1;
						s = va_arg(ap, char*);
						if(type==1)
						s=(char *)s;
						if ( s == 0 ) s = "(null char)";
						if (! pfield > 0 ) {
							pfield = str_len(s);
						}
						i = field - pfield;
						if ( i > 0 && !is_flag(L_ADJUST) ) { /* right adjusted */
							while ( i-- ) {
								if ( is_flag(ZERO_PAD) ) {
									mwrite('0');
								} else {
									mwrite(' ');
								}
							}
						}
						while( pfield-- ) {
							if ( (c = *s++) == '\0' ) break;
							mwrite(c);
						}
						if ( i > 0 && is_flag(L_ADJUST) ) {
							while ( i-- ) { /* left adjusted */
								mwrite(' ');
							}
						}
						break;
					case 'p':
						done = 1;
						set_flag(ALT_FORM|VOID_PTR);
						pfield = 8;
						base = 'x';
						goto number;
					case 'D':
						done = 1;
						set_flag(LONG_INT|SIGNED);
						base = 'd';
						goto number;
					case 'O':
						done = 1;
						set_flag(LONG_INT);
						base = 'o';
						goto number;
					case 'U':
						done = 1;
						set_flag(LONG_INT);
						base = 'd';
number:
						/* this is the correct way to extract va_args */
						if ( is_flag(SIGNED) ) {
							l = is_flag(LONG_INT) ? va_arg(ap, long) : 
								is_flag(SHORT_INT) ? (long)(short)va_arg(ap, int) :
								(long)va_arg(ap, int);
							format_long(numbuf, (unsigned long)l, base, pfield, sign, flags);
						} else {
							ul= is_flag(LONG_INT) ? va_arg(ap, unsigned long) : 
								is_flag(SHORT_INT) ? (unsigned long)(unsigned short)va_arg(ap, int):
								is_flag(VOID_PTR) ? (unsigned long)va_arg(ap, void*):
								(unsigned long)va_arg(ap, unsigned int);
							format_long(numbuf, ul, base, pfield, sign, flags);
						}
						/* now we have the padded, signed value in numbuf */
						/* now take care of field, alternate form, left adjust, zero pad */
						i = 0;
						if ( is_flag(ALT_FORM) && 
								!( numbuf[0] == '0' && numbuf[1] == '\0' ) ) {
							/* prepend only if != 0 */
							switch (base) {
								case 'x': case 'X':
									i = 2;
									break;
								case 'o':
									i = 1;
									break;
							}
						}
						tlen = str_len(numbuf);
						pfield = field - tlen - i;
						if ( pfield > 0 && !is_flag(L_ADJUST) ) { /* right adjusted */
							while ( pfield-- ) {
								if ( is_flag(ZERO_PAD) ) {
									mwrite('0');
								} else {
									mwrite(' ');
								}
							}
						}
						if ( i != 0 ) { /* print alternate form */
							mwrite('0');
							if ( i == 2 ) { 
								mwrite(base);
							}
						}
						for ( i = 0; i < (int) tlen; i++ ) {
							mwrite(numbuf[i]);
						}
						if ( pfield > 0 && is_flag(L_ADJUST) ) {
							while ( pfield-- ) { /* left adjusted only with spaces */
								mwrite(' ');
							}
						}
						/* finaly all is written, move to next */
						break;
floatnumber:        t = va_arg(ap, double);
                    pfield = 16;
	                format_float(numbuf, t, pfield, NULL, flags);
	                tlen = str_len(numbuf);
	                for ( i = 0; i < (int) tlen; i++ ) {
							mwrite(numbuf[i]);
						}
	                break;
					default: 
						done = 1;
						mwrite(*fmt);
						if ( *fmt == '\0') return written-1; /* don't count '\0' */
						break;
				}
			} while ( !done );
			fmt++;
		} else {
			mwrite(*fmt);
			fmt++;
		}
	}
	mwrite('\0');
	return written-1; /* don't count '\0' */
}


int printf_k(char* fmt, ...)
{
	va_list ap;
	unsigned long len;
	char buf[160];

	va_start(ap, fmt);
	len = myvsnprintf(buf, 160, fmt, ap,0);
	ioConsolePutString(buf);
	va_end(ap);

	return len;
}

void VideoInit() 
{
	vWindow.X1 = 0; vWindow.Y1 =  0;
  	vWindow.X2 = LCDWIDTH-1; vWindow.Y2 = LCDHEIGHT-2; // preserve space for status line
  	AtCursor.X = vWindow.X1; 
  	AtCursor.Y = vWindow.Y1; 
	AtCursor.TextAttr=0x07;
	ViewPortX=0;
	ViewPortY=0;
}


