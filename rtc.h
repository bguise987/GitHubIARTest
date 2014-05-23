#ifndef RTC_H
#define RTC_H

#define SQW_IN        //P1.6
#define RTC_IRQ       //P1.7
#define RTC_OUT       //P3.1
#define RTC_IN        //P3.2
#define RTC_CLK       //P3.3
#define RTC_CS        //P2.5
#define RTC_CS_LOW()    P2OUT &= ~BIT5;
#define RTC_CS_HIGH()   P2OUT |= BIT5;
#define RTC_INIT()      P3SEL |= BIT3 | BIT2 | BIT1;

//#define DIGIT(s, no) ((s)[no] - '0')

//#pragma inline
//void write_RTC(void);
void RTC_init(void);
void read_RTC(unsigned long address);

#endif