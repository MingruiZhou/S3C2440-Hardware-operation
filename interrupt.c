#include "s3c24xx.h"

//extern void I2CIntHandle(void);

#define GPF4_out    (1<<(4*2))
#define GPF5_out    (1<<(5*2))
#define GPF6_out    (1<<(6*2))

#define GPF4_msk    (3<<(4*2))
#define GPF5_msk    (3<<(5*2))
#define GPF6_msk    (3<<(6*2))

/*
 * S2,S3,S4对应GPF0、GPF2、GPG3
 */
#define GPF0_eint     (0x2<<(0*2))
#define GPF2_eint     (0x2<<(2*2))
#define GPG3_eint     (0x2<<(3*2))

#define GPF0_msk    (3<<(0*2))
#define GPF2_msk    (3<<(2*2))
#define GPG3_msk    (3<<(3*2))

void (*isr_handle_array[50])(void);

void Dummy_isr(void)
{
    while(1);
}

/*void init_irq(void)
{
    int i = 0;

    GPFCON &= ~(GPF4_msk | GPF5_msk | GPF6_msk);
    GPFCON |= GPF4_out | GPF5_out | GPF6_out;

    // S2,S3对应的2根引脚设为中断引脚 EINT0,ENT2
    GPFCON &= ~(GPF0_msk | GPF2_msk);
    GPFCON |= GPF0_eint | GPF2_eint;

    // S4对应的引脚设为中断引脚EINT11
    GPGCON &= ~GPG3_msk;
    GPGCON |= GPG3_eint;
    
    // 对于EINT11，需要在EINTMASK寄存器中使能它
    EINTMASK &= ~(1<<11);
        

    PRIORITY = (PRIORITY & ((~0x01) | (0x3<<7))) | (0x0 << 7) ;

    // EINT0、EINT2、EINT8_23使能
    INTMSK   &= (~(1<<0)) & (~(1<<2)) & (~(1<<5));

    for (i = 0; i < sizeof(isr_handle_array) / sizeof(isr_handle_array[0]); i++)
    {
        isr_handle_array[i] = Dummy_isr;
    }

    //INTMOD = 0x0;	      // 所有中断都设为IRQ模式
    //INTMSK = BIT_ALLMSK;  // 先屏蔽所有中断

//	isr_handle_array[ISR_IIC_OFT]  = I2CIntHandle;
}
*/

void IRQ_Handle(void)
{	
    unsigned long oft = INTOFFSET;

    switch( oft )
    {
        case 0: 
        {   
            GPFDAT |= (0x7<<4);   
            GPFDAT &= ~(1<<4);     
            break;
        }
        case 2:
        {   
            GPFDAT |= (0x7<<4);   
            GPFDAT &= ~(1<<5);     
            break;
        }
        case 5:
        {   
            GPFDAT |= (0x7<<4);  
            GPFDAT &= ~(1<<6);     
            break;
        }
        default:
            break;
    }

    if( oft == 5 ) 
        EINTPEND = (1<<11);   
    SRCPND = 1<<oft;
    INTPND = 1<<oft;
}

#define	GPF4_out	(1<<(4*2))
#define	GPF5_out	(1<<(5*2))
#define	GPF6_out	(1<<(6*2))
#define	GPF4_msk	(3<<(4*2))
#define	GPF5_msk	(3<<(5*2))
#define	GPF6_msk	(3<<(6*2))
#define GPF0_eint     (0x2<<(0*2))
#define GPF2_eint     (0x2<<(2*2))
#define GPG3_eint     (0x2<<(3*2))
#define GPF0_msk    (3<<(0*2))
#define GPF2_msk    (3<<(2*2))
#define GPG3_msk    (3<<(3*2))

void init_led(void)
{
    GPFCON &= ~(GPF4_msk | GPF5_msk | GPF6_msk);
    GPFCON |= GPF4_out | GPF5_out | GPF6_out;
}

void init_irq( )
{
    GPFCON &= ~(GPF0_msk | GPF2_msk);
    GPFCON |= GPF0_eint | GPF2_eint;
    GPGCON &= ~GPG3_msk;
    GPGCON |= GPG3_eint;
    EINTMASK &= ~(1<<11);
    PRIORITY = (PRIORITY & ((~0x01) | (0x3<<7))) | (0x0 << 7) ;
    INTMSK   &= (~(1<<0)) & (~(1<<2)) & (~(1<<5));
}


