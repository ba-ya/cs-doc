#include "config.h"


unsigned char Device_Descriptor[18] = {0x12,0x01, 0x10, 0x01,0xDC,0x0,0x0,0x20,       
						               0x05, 0x80,0x00, 0x10,0x00,0x00,0,0,0,0x01          	
						              };
unsigned char Configuration_Descriptor_All[32] = {
		        	    9,2,0x20,0x00,1,1,0,0x80,0xfa,//配置描述符
                        9,4,0,0,2,0xDC,0xAC,0xBC,0, //接口描述符                  
					    7,5,0x81,0x02,0x20,0x00,0x0,//端口描述符            
						7,5,0x02,0x02,0x20,0x00,0x0  //端口描述符         
				     	};
bit caiji_start,usb_connected,usb_connected_stored,usb_configured_stored;
int bufout[256],gaptime,gl_reminder_inc_ms,i,j;
unsigned char   bmRequestType,usb_sof_counter,usb_configuration_nb,*pbuffer,endpoint_status[2];
////////////////////////////
void main (void)//主程序
{
  usb_task_init();//USB初始化
  EpEnable();//端口使能
  while(1)
  {  
  usb_task();//USB处理函数 
  for(i=0;i<100;i++);
  caiji();//数据采集处理函数
  }

}
////////////////////////////
void usb_task_init(void)
{ 
  USBCON |= 0x80; //使能USB控制器
  USBCON |= 0x10; /*USB软件插拔*/
  delay(100);
  USBCON &= ~0x10;
  PLLDIV = 32; //配置控制器时钟
  PLLCON |= 0x02;//使能PLL
  UEPNUM = 0;//复位控制端口
  UEPCONX = 0;
  /*初始化状态变量*/
  usb_connected = 0;
  usb_connected_stored = 1;
  usb_configured_stored = 0;
  endpoint_status[0] = 0x00;
  endpoint_status[1] = 0x00;
  usb_connected = 0;
  usb_configuration_nb = 0x00;
}
///////////////////////////////////
//////////////////////////////////////
void EpEnable(void)
{
	UEPNUM=0x00;	UEPCONX=0x80;//端口0
	UEPNUM=0x01;	UEPCONX=0x86;//端口1
	UEPNUM=0x02;	UEPCONX=0x82;//端口2
	UEPRST=0x07;	UEPRST= 0x00;//端口复位
	UEPIEN=0x07;	USBIEN|=0x01<<4;
	USBADDR=0x01<<7;
}
////////////////////////////////////////////////
void usb_task(void){
      if (UEPINT & EP0)usb_enumeration_process();
      if (UEPINT & EP1)   
	    {
	     Usb_clear_tx_complete();	//UEPINT地址0xF8却不能位寻址	
	    }
	   if (UEPINT & EP2)   
		 {              
         unsigned char  Bufin[64];  
         i = ReadEp(2,Bufin);
		 if(Bufin[1]&0x80!=0)
		  {
          caiji_start=1;//设置采集标志位
          gaptime= Bufin[0];//采集频率	
          if(Bufin[1]&0x7f==1)
            {
             P1_4=1;
             }
          if(Bufin[1]&0x7f==2)
             {
             P1_5=1;
             }
          if(Bufin[1]&0x7f==3)
             {
             P1_6=1;
             }
           if(Bufin[1]&0x7f==4)
             {
             P1_7=1;
             }                			
		   }
           else
           {
             caiji_start=0;//设置采集状态位
           }
    /*数据缓冲区清零*/             
	for(i=0;i<64;i++)
		{
		Bufin[i]=0;
		}
   }           
  }
/////////////////////////////////////////////////////////
////////////////////////////////////////////////
void usb_enumeration_process (void)//USB设备枚举过程
{ 
  UEPNUM = 0;//选择控制端口
  bmRequestType = UEPDATX;          /* 得到 bmRequestType */
  switch (UEPDATX)                  /* 判断 bRequest 的值 */
  {
    case GET_DESCRIPTOR:                   /*获得描述符请求*/
      usb_get_descriptor();
      break;
    case GET_CONFIGURATION:               /*获得配置请求*/
      usb_get_configuration();
      break;
    case SET_ADDRESS:                    /*设置地址请求*/
      usb_set_address();
      break;
    case SET_CONFIGURATION:              /*设置配置请求或设置HID报表请求*/
      if (bmRequestType == 0) { usb_set_configuration(); }
         break;
     default:
      UEPSTAX =UEPSTAX & ~0x04;        /*清除SETUP标志*/
      UEPSTAX =UEPSTAX | 0x20;         /*中止请求设置*/               
      while (!UEPSTAX & 0x08);
      UEPSTAX =UEPSTAX &~ 0x20;
      UEPSTAX =UEPSTAX &~ 0x08;
      break;
    }
    UEPSTAX = UEPSTAX &~0x80;
}
//////////////////////////////////////////////////////
void usb_set_address (void)                 /*设置设备地址子程序*/
{
  unsigned char address;
  address = UEPDATX;                    /* 获得系统分配的设备地址 */
  UEPSTAX &= ~0x04;                     /*清除SETUP标志*/
  UEPSTAX |= 0x10;                       /* 返回0字节状态字 */
  USBCON |= 0x01;                         /*设置地址使能*/
  while (!(UEPSTAX & 0x01));
  UEPSTAX=UEPSTAX & ~0x01;
  USBADDR = (0x80 | address);              /*配置设备地址*/
}
////------set_config-------////////
void usb_set_configuration (void)
{
  unsigned char configuration_number;
  configuration_number = UEPDATX;   /* 读取配置数*/
  UEPSTAX &= ~0x80;
  UEPSTAX &= ~0x04;                   /*清除SETUP标志*/
  if (configuration_number <= 1)
  {
    usb_configuration_nb = configuration_number;
  }
  else
  {
    UEPSTAX |= 0x20;            /*中止请求设置*/ 
    while (!UEPSTAX & 0x08);
    UEPSTAX &= ~0x20;
    UEPSTAX &= ~0x08;
    return;
  }

  UEPSTAX |= 0x10;        /* 返回0字节状态字*/
  while (!UEPSTAX & 0x01);
  UEPSTAX &= ~0x01;
  /* 设备端口配置 */
  UEPNUM = 1;
  UEPCONX = 0x87;
  UEPRST = 0x01;
  UEPRST = 0x00;
}
///////////////////////////////////////////////////////////////
void usb_get_descriptor (void)
{
  unsigned char   data_to_transfer;
  unsigned  int  wLength;
  unsigned char   descriptor_type;
  unsigned char   string_type;                      
  string_type = UEPDATX;            /* 读取 wValue的低位 */
  descriptor_type = UEPDATX;        /* 读取 wValue的高位 */
  switch (descriptor_type)          /*判断描述符类型*/
  {
    case DEVICE:                            /*设备描述符�*/                
    {
      data_to_transfer = sizeof (Device_Descriptor);
      pbuffer = &(Device_Descriptor[0]);//指向设备描述符结构首地址
      break;
    }

    case CONFIGURATION:                       /*配置*/
    {
      data_to_transfer = sizeof (Configuration_Descriptor_All);
      pbuffer = &(Configuration_Descriptor_All[0]);
      break;
   } 
    default:
    {
      UEPSTAX &= ~0x04;
      UEPSTAX |= 0x20;
      while ((!(UEPSTAX & 0x08)) && (UEPSTAX & 0x04));
      UEPSTAX &= ~0x08;
      UEPSTAX &= ~0x20;
      UEPSTAX &= ~0x80;
      return;
    }
  }

  ACC = UEPDATX;                   
  ACC = UEPDATX;
  ((unsigned char*)&wLength)[1] = UEPDATX;   /* 读取要传输的长度 */
  ((unsigned char*)&wLength)[0] = UEPDATX;
  if (wLength > data_to_transfer);     /* 读取的长度大于描述符长度时 */      
  else
  {
    data_to_transfer = (unsigned char)wLength;       /*传送需要的数据长度 */
  }
  UEPSTAX &= ~0x04 ;                    
  UEPSTAX |= 0x80;                            
  while (data_to_transfer > 32)/*传送的长度大于控制端口的长度时*/
  {
    pbuffer = usb_send_ep0_packet(pbuffer, 32);/*传送一次端口长度的数据*/
    data_to_transfer -= 32;
    while ((!(UEPSTAX & 0x42)) && (!(UEPSTAX & 0x01)));/*传输没有完成*/
    UEPSTAX=UEPSTAX & 0x01;
    if ((UEPSTAX & 0x42))               
    {
      UEPSTAX &= ~0x10;
      UEPSTAX &= ~0x02;
      return;
    }
  }
  /* 传送最后一次数据 */
  pbuffer = usb_send_ep0_packet(pbuffer, data_to_transfer);
  data_to_transfer = 0;
  while ((!(UEPSTAX & 0x42)) && (!(UEPSTAX & 0x01)));
  UEPSTAX=UEPSTAX & 0x01;
  if ((UEPSTAX & 0x42))                  /* if cancel from USB Host */
  {
    UEPSTAX &= ~0x10;
    UEPSTAX &= ~0x02;
    return;
  }
}
//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned int caiji_single()
{//启动采样
   unsigned int dd;
   P1_0=0;
   P1_1=0;
   for(i=0;i<10;i++);
   P1_0=1;
   P1_1=1;
  //查询转换是否结束
  if(P1_3==1);
   //读取转换数据
  P1_0=0;
  P1_2=0;
  dd=P1;
  return dd;
}
//////////////////////////////////////////////////////
void caiji()
{
if(caiji_start == 1)
{
for(i=0;i<256;i++)
{
bufout[i]=caiji_single();
delay(gaptime);
}
for(i=0;i<100;i++);
senddata();
}
}
/////////////////////////////////

void senddata()
{

for(i=0;i<8;i++)
{
 for(j=0;j<32;j++)
{
  Usb_write_byte(bufout[i*32+j]);
}
Usb_set_tx_ready();
while(!Usb_tx_complete());
Usb_clear_tx_complete();
}
}
/////////////////////////////

void delay(int mm)
{
for(i=0;i<mm;i++);
}
