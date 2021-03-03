#include <unistd.h>  
#include <signal.h>  
#include <stdio.h>  
#include <iobb.h>  
  
#define SPI_BUS SPI0  
  
int not_finished=1;  
  
void  
check_root_user(void)  
{  
  if(geteuid()!=0)  
    printf("Run as root user! (or use sudo)\n");  
}  
  
void  
ctrl_c_handler(int dummyvar)  
{  
  not_finished=0;  
}  
  
int  
main(void)  
{  
  signal (SIGINT, ctrl_c_handler);  
  check_root_user();
  iolib_init();
  if(not_finished){
  int i;  
  int rxval;    
  configure_spi_pins(17, 18, 21, 22); // CS, D1, D0, CLK  
  if (spi_enable(SPI_BUS))  
  {  
    spi_ctrl(SPI_BUS, SPI_CH0, SPI_MASTER, SPI_RXTX, /* use SPI_TX(write) or SPI_RX(read) or SPI_RXTX(r/w) depending on needs */  
             SPI_DIV32, SPI_CLOCKMODE0, /* 48 MHz divided by 32 which gives 1.5MHz for 1.8MHz max in MCP3202; SPI mode 0 for MCP3202 */  
             SPI_CE_ACT_LOW, SPI_OUTIN, 24); /* D0 is output and D1 is input, opposite to default, use SPI_INOUT for default; 24-bit transactions */  
  }  
  else  
  {  
    printf("error, spi is not enabled\n");  
    iolib_free();  
    return(1);  
  }  
  while(1)   
  {  
    spi_transact(SPI_BUS, SPI_CH0, 0x553caa /*or integer to transmit data*/, &rxval);  
    printf("Received: 0x%04x \n", rxval&0xffff);  
    iolib_delay_ms(500);  
  }  
  spi_disable(SPI_BUS);  
  }
  iolib_free();  
  return(0);  
}
