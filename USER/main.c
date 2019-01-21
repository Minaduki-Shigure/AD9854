#include "sys.h"
#include "key.h"
#include "adc.h"
#include "lcd.h"
#include "math.h"
#include "delay.h"
#include "delay.h"
#include "usart.h"
#include "timer.h"
#include "AD9854.h"

#define A 0.75

#define X_1M 21
#define X_200k 1
#define X_40M 220

#define Y_0dB 20
#define Y_10dB 50
#define Y_20dB 80
#define Y_30dB 110
#define Y_40dB 140

#define Y_100 180
#define Y_0	240
#define Y_100_ 300

int draw;
int first=1;
int clearend;

int maxMHz;
double maxdB;
					
double scaledB;
int startflag=0;
int start;
int end;

float Z1=0;
float Z2=0;

//1MHz-25MHz 266.56 - 2.35077 x
//25MHz-29MHz 212
//29MHz-40MHz 146.036 + 2.32727 x
//拟合2
//1-28MHz 268.74-2.2x
//30-40MHz 153+2x


//默认为扫频测量。此时KEY0增加频率步进值，KEY1降低频率步进值。
//再按WK_UP，此时KEY0增加起始扫频频率，KEY1降低起始扫频频率。
//再按WK_UP，此时KEY0增加结束扫频频率，KEY1降低结束扫频频率。
//再按切换点频测量。点频测量时,按KEY0增加频率，KEY1降低频率。
//再按，相当于调零，此时的输出将被置0，等待测试。
//KEY0、KEY1、WK_UP为0代表按下。

//单音模式下，默认SPI直接输出当前频率，当前两个ADC的读入值。
//扫频模式下，当检测到扫频为1M-40M步进100kHz扫频时，输出两个ADC读入值。

 int main(void)
 {
	 //一切频率单位都为hkHz，百千赫兹
	 double freq;
	 int lcd_x,lcd_y1,lcd_y2;
	 int StartFreq=10,StopFreq=400,FreqStep=1;
	 double DC1=2,DC2=3;
	 double Amp,Phase;
	 int xMHz;
//	 int AmpZero[391]={0};
	 int ZeroSet=0;
	 int SingleFreq=200;	//默认单音输出20MHz
	 int mission=0;	//用于监测WK_UP按了多少次，从而确定此时两个KEY的功能。需要防抖来防止抖动导致不知道按了几次。
   const u8 WAIT=1,DLY=2,TEST=3,EXE=4,WAIT_UP=5;
	 u8 state1=WAIT,state2=WAIT,state3=WAIT,c=0,d=0,e=0;//u8 无符号8位数，0-255
	 //0:改频率步进值
	 //1:改起始扫频频率
	 //2：改终止扫频频率
	 //3：点频测量
	 //4+：归零
	 double fit;
	 
	 SystemInit(); 	//系统时钟设置
	 delay_init();	    	 //延时函数初始化	  
	 uart_init(9600);	 //串口初始化为9600
	 AD9854Init();
	 KEY_Init();
	 Adc_Init();
	 Adc2_Init();
	 LCD_Init();
	 GPIO_ResetBits(GPIOA,GPIO_Pin_8);
	 
		LCD_DrawLine(1,160,240,160);
		LCD_DrawLine(20,140,220,140);
		LCD_DrawLine(20,140,20,20);
		LCD_DrawLine(20,300,220,300);
		LCD_DrawLine(20,300,20,180);
		
		for(draw=0;draw<=20;draw++)
		{	
			LCD_DrawLine(20+draw*10,140,20+draw*10,142);
			LCD_DrawLine(20+draw*10,300,20+draw*10,302);
		}
		
		for(draw=0;draw<=12;draw++)
		{	
			LCD_DrawLine(18,20+draw*10,20,20+draw*10);
			LCD_DrawLine(18,180+draw*10,20,180+draw*10);
		}
		
		LCD_ShowString(208,145,160,12,12,"40MHz");
		LCD_ShowString(208,305,160,12,12,"40MHz");
		LCD_ShowString(15,145,160,12,12,"1MHz");
		LCD_ShowString(15,305,160,12,12,"1MHz");
		LCD_ShowString(112,145,160,12,12,"20MHz");
		LCD_ShowString(112,305,160,12,12,"20MHz");
		LCD_ShowString(63,145,160,12,12,"10MHz");
		LCD_ShowString(63,305,160,12,12,"10MHz");
		LCD_ShowString(160,145,160,12,12,"30MHz");
		LCD_ShowString(160,305,160,12,12,"30MHz");
		
		LCD_ShowString(15,6,160,12,12,"dB");
		LCD_ShowString(5,166,160,12,12,"Degree");
		LCD_ShowString(8,15,160,12,12,"0");
		LCD_ShowString(1,45,160,12,12,"-10");
		LCD_ShowString(1,75,160,12,12,"-20");
		LCD_ShowString(1,105,160,12,12,"-30");
		LCD_ShowString(1,135,160,12,12,"-40");
		LCD_ShowString(5,185,160,12,12,"90");
		LCD_ShowString(5,202,160,12,12,"60");
		LCD_ShowString(5,218,160,12,12,"30");
		LCD_ShowString(8,235,160,12,12,"0");
		LCD_ShowString(1,252,160,12,12,"-30");
		LCD_ShowString(1,268,160,12,12,"-60");
		LCD_ShowString(1,285,160,12,12,"-90");
	 
	 
	 while(1){
	 for(xMHz=StartFreq;xMHz<=StopFreq;xMHz+=FreqStep){
//		 printf("DD");
//		 printf("K:%d T:%d B:%d m:%d s:%d\n",StartFreq,StopFreq,FreqStep,mission,SingleFreq);
//		 printf("%lf A %d\n",fit,mission);
		 
		 //WAKE_UP防抖
		 if(state1==WAIT){
			 if(WK_UP==1) state1=DLY;}
		 if(state1==DLY){
			 c++;
			 if(c==10){
				 c=0;
				 state1=TEST;
			 }
		 }
		 if(state1==TEST){
			 if(WK_UP==0) state1=WAIT;
			 if(WK_UP==1) state1=EXE;}
		 if(state1==EXE){		//需要执行的代码
			 mission++;
			 //mission=0 扫频，步进设置
			 //mission=1 扫频，开始频率设置
			 //mission=2 扫频，结束频率设置
			 //mission=3 单音，频率设置
			 //mission=4 扫频调零
			 if(mission>4) mission=0;
			 state1=WAIT_UP;
			 LCD_Fill(21,21,219,139,WHITE);
		 }
		 if(state1==WAIT_UP){//没有则按键功能会被执行多次
			 if(WK_UP==0) state1=WAIT;
		 }

//		 if(WK_UP==1){
//			 mission++;
//			 if(mission>3) mission=0;
//		 }
		 
		 //KEY0防抖
		 if(state2==WAIT){
			 if(KEY0==1) state2=DLY;}
		 if(state2==DLY){
			 d++;
			 if(d==10){
				 d=0;
				 state2=TEST;
			 }
		 }
		 if(state2==TEST){
			 if(KEY0==0) state2=WAIT;
			 if(KEY0==1) state2=EXE;}
		 if(state2==EXE){	//确认按下，输入需要执行的代码
			 if(mission==0){
				 FreqStep+=5;
			   if(FreqStep>51) FreqStep=1;	//最大步进5MHz
			 }
			 else if(mission==1){
				 StartFreq+=10;
			   if(StopFreq-FreqStep<StartFreq) StartFreq=StopFreq-FreqStep;	//最小间距1个步进
			 }
			 else if(mission==2){
				 StopFreq+=10;
			   if(StopFreq>400) StopFreq=400;	//最大值40MHz
			 }
			 else if(mission==3){
				 SingleFreq+=10;
			   if(SingleFreq>400) SingleFreq=400;	//最大40MHz
			 }
			 else if(mission==4){
				 ZeroSet=1;
			 }
			 else mission=0;
			 state2=WAIT_UP;
		 }
		 if(state2==WAIT_UP){//没有则按键功能会被执行多次
			 if(KEY0==0) state2=WAIT;
		 }
		 
		 //KEY1防抖
		 if(state3==WAIT){
			 if(KEY1==1) state3=DLY;}
		 if(state3==DLY){
			 e++;
			 if(e==10){
				 e=0;
				 state3=TEST;
			 }
		 }
		 if(state3==TEST){
			 if(KEY1==0) state3=WAIT;
			 if(KEY1==1) state3=EXE;}
		 if(state3==EXE){	//确认按下，输入需要执行的代码
			 if(mission==0){
			   FreqStep-=5;
			   if(FreqStep<1) FreqStep=1;	//最大步进5MHz
			 }
			 else if(mission==1){
				 StartFreq-=10;
			   if(StartFreq<10) StartFreq=10;	//最小值1MHz
			 }
			 else if(mission==2){
				 StopFreq-=10;
			   if(StartFreq+FreqStep>StopFreq) StopFreq=StartFreq+FreqStep;	//最小间距1个步进
			 }
			 else if(mission==3){
				 SingleFreq-=10;
			   if(SingleFreq<10) SingleFreq=10;	//最低1MHz
			 }
			 else mission=0;
			 state3=WAIT_UP;
		 }
		 if(state3==WAIT_UP){//没有则按键功能会被执行多次
			 if(KEY1==0) state3=WAIT;
		 }
		 

		 
			//幅度相位计算
		 DC1=Get_Adc_Average(ADC_Channel_2,5);	//获取上一个ADC值
		 DC2=Get_Adc2_Average(ADC_Channel_10,5);	//获取下一个ADC值
		 DC1=3.3*(DC1/4095);
		 DC2=3.3*(DC2/4095);
		 //LCD_ShowFloat(40,160,DC1,5,9,16);
		 //LCD_ShowFloat(40,180,DC2,5,9,16);
		 DC1=(2-DC1)/5-Z1;
		 DC2=(2-DC2)/5-Z2;
		 //LCD_ShowFloat(40,200,DC1,5,9,16);
		 //LCD_ShowFloat(40,220,DC2,5,9,16);
//			vi=3300*(adc1/4095);
//			vq=3300*(adc2/4095);
//			H_raw=(2*sqrt(vi*vi+vq*vq))/(AMP*AMP);
//			H_dB=20*log(H_raw);
//			phase=atan(vqi);
//			phase_d=phase*(180/3.14);
//			lcd_x=xMHz/2+X_1M-1;
			Amp=2*sqrt(DC1*DC1+DC2*DC2);
			Amp=Amp/(A*A);
			Amp=20*log10(Amp);
			//LCD_ShowFloat(40,240,Amp,5,9,16);
		  Phase=atan(-DC2/DC1);
			Phase=Phase*180/3.14;
			if(DC1<0&&DC2<0) Phase=Phase+90;
			if(DC1<0&&DC2>0) Phase=Phase-90;
			//LCD_ShowFloat(40,260,Phase,5,9,16);
			
			//喂数据
			//Amp=0.1*xMHz-40;
		  //Phase=100-xMHz/2;
			if(ZeroSet==1&&xMHz==10) ZeroSet=2;
			if(ZeroSet==2&&xMHz==400) ZeroSet=0;
			if(ZeroSet==2){
		//		AmpZero[xMHz-10]=Amp;
	//			PhaseZero[xMHz-10]=Phase;
			}
		//	Amp=Amp-AmpZero[xMHz-10];
	//		Phase=Phase-PhaseZero[xMHz-10];
			
			if(mission!=3)
			{
				if(Amp>maxdB)
				{
					maxMHz=xMHz;
					maxdB=Amp;
				}
				if(startflag==0)
				{
					if(Amp>scaledB)
					{
						startflag=1;
						start=xMHz;
					}
				}
				if(startflag==1)
				{
					if(Amp<scaledB)
					{
						startflag=2;
						end=xMHz;
					}
				}
						
				
				lcd_x=xMHz/2+X_1M-1;
				lcd_y1=20-3*Amp;
				lcd_y2=240-(float)0.6*Phase;
				//LCD_ShowFloat(40,260,lcd_y1,5,9,16);
				
				LCD_DrawPoint(lcd_x,lcd_y1);
				LCD_DrawPoint(lcd_x,lcd_y2);
				clearend=lcd_x+30;
				if(clearend>X_40M)
					clearend=X_40M;
				LCD_Fill(lcd_x+1,20,clearend,139,WHITE);
				LCD_Fill(lcd_x+1,180,clearend,299,WHITE);
				if(lcd_x>=StopFreq/2+X_1M-1)
				{
					//delay_ms(500);
					LCD_ShowString(40,40,200,16,16,"Centre: ");
					LCD_ShowString(40,60,200,16,16,"Start:  ");
					LCD_ShowString(40,80,200,16,16,"End:");
					LCD_ShowString(40,100,200,16,16,"Scale:");
					LCD_ShowNum(100,40,maxMHz*100000,9,16);
					LCD_ShowNum(100,60,start*100000,9,16);
					LCD_ShowNum(100,80,end*100000,9,16);
					LCD_ShowNum(100,100,(end-start)*100000,9,16);			
					//delay_ms(5000);
					
					scaledB=maxdB-3;
					maxdB=-100;
					start=10;
					end=400;
					startflag=0;
					
					LCD_Fill(21,20,StartFreq+30,139,WHITE);
					LCD_Fill(21,180,StartFreq+30,299,WHITE);
				}
			}
			
		 //频率幅值调整
		 freq=xMHz*100000;	//乘100kHz即得实际所需频率
		 if(mission==3)		 
		 {
				freq=SingleFreq*100000;	//乘100kHz即得实际所需频率
				LCD_ShowString(40,40,200,16,16,"Freq: ");
				LCD_ShowString(40,60,200,16,16,"Amp:  ");
				LCD_ShowString(40,80,200,16,16,"Phase:");
				LCD_ShowNum(100,40,freq,9,16);
				LCD_ShowFloat(120,60,Amp,2,9,16);
				LCD_ShowFloat(120,80,Phase,2,9,16);
		 }
		 AD9854SetAmp(3000,3000);
		 AD9854WriteFreqSingle(freq);	//单音输出频率
	 }
  }
 }
 
 		 //第二版拟合曲线
//		 if(mission!=3){
//				if(xMHz<290){
//					fit=750000/(268.74-0.22*xMHz);		//强行调整输出
//				}
//				else if(xMHz>290){
//					fit=750000/(153+0.2*xMHz);		//强调输出
//				}
//				else{
//					fit=4075;
//				}
//				if(fit>4095) fit=4095;		//如果溢出，那么将fit置为可用最大值
//		}
//		 else{
//				if(SingleFreq<290){
//					fit=750000/(268.74-0.22*SingleFreq);		//强行调整输出
//				}
//				else if(SingleFreq>290){
//					fit=750000/(153+0.2*SingleFreq);		//强调输出
//				}
//				else{
//					fit=4075;
//				}
//				if(fit>4095) fit=4095;		//如果溢出，那么将fit置为可用最大值
//		 }
				//第一版拟合曲线
//			if(mission!=3){
//				if(xMHz<250){
//					fit=864000/(267-0.235*xMHz);		//强行调整输出
//				}
//				else if(xMHz>290){
//					fit=864000/(146+0.233*xMHz);		//强调输出
//				}
//				else{
//					fit=4075;
//				}
//				if(fit>4095) fit=4095;		//如果溢出，那么将fit置为可用最大值
//			}
//			else{
//				if(SingleFreq<250){
//					fit=864000/(267-0.235*SingleFreq);		//强行调整输出
//				}
//				else if(SingleFreq>290){
//					fit=864000/(146+0.233*SingleFreq);		//强调输出
//				}
//				else{
//					fit=4075;
//				}
//				if(fit>4095) fit=4095;		//如果溢出，那么将fit置为可用最大值
//			}
 