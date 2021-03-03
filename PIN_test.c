#include <stdio.h>  
#include <unistd.h>  
#include <signal.h>  
#include <iobb.h>  
  
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
  iolib_setdir(9, 14, DigitalOut);  
  while(1)  
  {  
    pin_high(9, 14);  
    iolib_delay_ms(800);  
    pin_low(9, 14);  
    iolib_delay_ms(800);
  }  
  iolib_free();  
  return(0); 
}