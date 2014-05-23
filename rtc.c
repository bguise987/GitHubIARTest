#include "rtc.h"
//#include <msp430x53x.h>
#include "io430.h"

#pragma inline
unsigned char send_byte(const unsigned char byte) {
  while (UCB0STAT & UCBUSY);
  UCB0TXBUF = byte;
  while (UCB0STAT & UCBUSY);
  return (UCB0RXBUF);
}

void RTC_init(void)
{
  unsigned char ht_bit;
  unsigned char st_bit;
  
  P2DIR |= BIT5;
  RTC_CS_HIGH();

  P3SEL |= BIT1 | BIT2 | BIT3;
  
  UCB0CTL1 |= UCSWRST;
  UCB0CTL0 |= UCMST | UCSYNC | UCMSB | UCCKPH;  /* Master Mode, Synchronous, MSB first, Read first edge/Change second*/ 
  UCB0CTL1 |= UCSSEL_2;                         /* SMCLK */
  UCB0BR0 = 0x01;
  UCB0BR1 = 0x00;                               /* Prescaler = /1 */
  UCB0CTL1 &= ~UCSWRST;                         /* Release reset for operation */

  
  // Now clear the HT bit
  RTC_CS_LOW();
  send_byte(0x8C);
  send_byte(0x00);
  RTC_CS_HIGH();
  
  // Reset Stop Bit to 1 then 0, as per 
  //https://github.com/tomorrowlab/M41T93-Real-Time-Clock/blob/master/RTC_M41T93.cpp
  RTC_CS_LOW();
  send_byte(0x01);      // Address of ST bit
  st_bit = send_byte(0x00);
  RTC_CS_HIGH();
  
  RTC_CS_LOW();
  send_byte(0x81);
  send_byte(0x80);      // Set ST bit high
  RTC_CS_HIGH();
  
  RTC_CS_LOW();
  send_byte(0x81);      // Address of ST bit
  send_byte(0x00);      // Set ST bit low
  RTC_CS_HIGH();
  
  RTC_CS_LOW();
  send_byte(0x81);      // Address of ST bit
  send_byte(st_bit & 0x7F);     // Set seconds register back with ST bit low
  RTC_CS_HIGH();



  
  
  /* Setup DMA for RTC transfers
   *
   * The DMA will work as follows:
   * -DMA0 will read the data coming from the SPI buffer
   * -DMA1 will send a byte out, triggering the next data read
   */
  DMACTL0 &= ~(DMA1TSEL_15 | DMA0TSEL_15);  /* Clear triggers */
  DMACTL0 |= DMA1TSEL_12 | DMA0TSEL_12;     /* Trigger on USCI B0 Interrupts */
  DMACTL1 &= ~ROUNDROBIN;                   /* Ensure DMA0 priority is higher than DMA1 */
  DMA0SA = UCB0RXBUF_;                      /* Source USCI B0 RX buffer */
  DMA0SZ = 8;                               /* Set size to 8 bytes */
  DMA0CTL =
    DMAIE   |                         /* Enable interrupt */
    DMADT_0 |                         /* Single transfer mode */
    DMASBDB |                         /* Byte mode */
    DMADSTINCR_3;                     /* Increment the destination address */

  DMA1SA = UCB0TXBUF_;                /* Source USCI */
  DMA1DA = UCB0TXBUF_;
  DMA1SZ = 7;
  
  DMA1CTL =
    DMADT_0 |                         /* Single transfer mode */
    DMASBDB;                          /* Byte mode */
}

void read_RTC(unsigned long address)
{
  DMA0DA = address;
  IFG2 &= ~UCB0RXIFG;
  RTC_CS_LOW();
  send_byte(0x00);
  DMA0CTL |= DMAEN;
  DMA1CTL |= DMAEN;
  
  /* Kick off the transfer by sending the first byte */
  //UCB0TXBUF = 0xFF;
  // Let's send something that the chip shouldn't think is a write command
  UCB0TXBUF = 0x00;
  

  _BIS_SR(LPM0_bits | GIE);

  _NOP();
  RTC_CS_HIGH();
  _NOP();
}

#pragma vector=DMA_VECTOR
__interrupt void DMA_ISR(void)
{
  switch(__even_in_range(DMAIV, 6)) {
  case DMAIV_DMA0IFG:
    DMA0CTL &= ~DMAIFG;
    break;
  case DMAIV_DMA2IFG:
    DMA2CTL &= ~(DMAIFG | DMAIE);
    break;
  }
  _BIC_SR_IRQ(LPM0_bits);
}