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
//���2
//1-28MHz 268.74-2.2x
//30-40MHz 153+2x


//Ĭ��ΪɨƵ��������ʱKEY0����Ƶ�ʲ���ֵ��KEY1����Ƶ�ʲ���ֵ��
//�ٰ�WK_UP����ʱKEY0������ʼɨƵƵ�ʣ�KEY1������ʼɨƵƵ�ʡ�
//�ٰ�WK_UP����ʱKEY0���ӽ���ɨƵƵ�ʣ�KEY1���ͽ���ɨƵƵ�ʡ�
//�ٰ��л���Ƶ��������Ƶ����ʱ,��KEY0����Ƶ�ʣ�KEY1����Ƶ�ʡ�
//�ٰ����൱�ڵ��㣬��ʱ�����������0���ȴ����ԡ�
//KEY0��KEY1��WK_UPΪ0�����¡�

//����ģʽ�£�Ĭ��SPIֱ�������ǰƵ�ʣ���ǰ����ADC�Ķ���ֵ��
//ɨƵģʽ�£�����⵽ɨƵΪ1M-40M����100kHzɨƵʱ���������ADC����ֵ��

 int main(void)
 {
	 //һ��Ƶ�ʵ�λ��ΪhkHz����ǧ����
	 double freq;
	 int lcd_x,lcd_y1,lcd_y2;
	 int StartFreq=10,StopFreq=400,FreqStep=1;
	 double DC1=2,DC2=3;
	 double Amp,Phase;
	 int xMHz;
//	 int AmpZero[391]={0};
	 int ZeroSet=0;
	 int SingleFreq=200;	//Ĭ�ϵ������20MHz
	 int mission=0;	//���ڼ��WK_UP���˶��ٴΣ��Ӷ�ȷ����ʱ����KEY�Ĺ��ܡ���Ҫ��������ֹ�������²�֪�����˼��Ρ�
   const u8 WAIT=1,DLY=2,TEST=3,EXE=4,WAIT_UP=5;
	 u8 state1=WAIT,state2=WAIT,state3=WAIT,c=0,d=0,e=0;//u8 �޷���8λ����0-255
	 //0:��Ƶ�ʲ���ֵ
	 //1:����ʼɨƵƵ��
	 //2������ֹɨƵƵ��
	 //3����Ƶ����
	 //4+������
	 double fit;
	 
	 SystemInit(); 	//ϵͳʱ������
	 delay_init();	    	 //��ʱ������ʼ��	  
	 uart_init(9600);	 //���ڳ�ʼ��Ϊ9600
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
		 
		 //WAKE_UP����
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
		 if(state1==EXE){		//��Ҫִ�еĴ���
			 mission++;
			 //mission=0 ɨƵ����������
			 //mission=1 ɨƵ����ʼƵ������
			 //mission=2 ɨƵ������Ƶ������
			 //mission=3 ������Ƶ������
			 //mission=4 ɨƵ����
			 if(mission>4) mission=0;
			 state1=WAIT_UP;
			 LCD_Fill(21,21,219,139,WHITE);
		 }
		 if(state1==WAIT_UP){//û���򰴼����ܻᱻִ�ж��
			 if(WK_UP==0) state1=WAIT;
		 }

//		 if(WK_UP==1){
//			 mission++;
//			 if(mission>3) mission=0;
//		 }
		 
		 //KEY0����
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
		 if(state2==EXE){	//ȷ�ϰ��£�������Ҫִ�еĴ���
			 if(mission==0){
				 FreqStep+=5;
			   if(FreqStep>51) FreqStep=1;	//��󲽽�5MHz
			 }
			 else if(mission==1){
				 StartFreq+=10;
			   if(StopFreq-FreqStep<StartFreq) StartFreq=StopFreq-FreqStep;	//��С���1������
			 }
			 else if(mission==2){
				 StopFreq+=10;
			   if(StopFreq>400) StopFreq=400;	//���ֵ40MHz
			 }
			 else if(mission==3){
				 SingleFreq+=10;
			   if(SingleFreq>400) SingleFreq=400;	//���40MHz
			 }
			 else if(mission==4){
				 ZeroSet=1;
			 }
			 else mission=0;
			 state2=WAIT_UP;
		 }
		 if(state2==WAIT_UP){//û���򰴼����ܻᱻִ�ж��
			 if(KEY0==0) state2=WAIT;
		 }
		 
		 //KEY1����
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
		 if(state3==EXE){	//ȷ�ϰ��£�������Ҫִ�еĴ���
			 if(mission==0){
			   FreqStep-=5;
			   if(FreqStep<1) FreqStep=1;	//��󲽽�5MHz
			 }
			 else if(mission==1){
				 StartFreq-=10;
			   if(StartFreq<10) StartFreq=10;	//��Сֵ1MHz
			 }
			 else if(mission==2){
				 StopFreq-=10;
			   if(StartFreq+FreqStep>StopFreq) StopFreq=StartFreq+FreqStep;	//��С���1������
			 }
			 else if(mission==3){
				 SingleFreq-=10;
			   if(SingleFreq<10) SingleFreq=10;	//���1MHz
			 }
			 else mission=0;
			 state3=WAIT_UP;
		 }
		 if(state3==WAIT_UP){//û���򰴼����ܻᱻִ�ж��
			 if(KEY1==0) state3=WAIT;
		 }
		 

		 
			//������λ����
		 DC1=Get_Adc_Average(ADC_Channel_2,5);	//��ȡ��һ��ADCֵ
		 DC2=Get_Adc2_Average(ADC_Channel_10,5);	//��ȡ��һ��ADCֵ
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
			
			//ι����
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
			
		 //Ƶ�ʷ�ֵ����
		 freq=xMHz*100000;	//��100kHz����ʵ������Ƶ��
		 if(mission==3)		 
		 {
				freq=SingleFreq*100000;	//��100kHz����ʵ������Ƶ��
				LCD_ShowString(40,40,200,16,16,"Freq: ");
				LCD_ShowString(40,60,200,16,16,"Amp:  ");
				LCD_ShowString(40,80,200,16,16,"Phase:");
				LCD_ShowNum(100,40,freq,9,16);
				LCD_ShowFloat(120,60,Amp,2,9,16);
				LCD_ShowFloat(120,80,Phase,2,9,16);
		 }
		 AD9854SetAmp(3000,3000);
		 AD9854WriteFreqSingle(freq);	//�������Ƶ��
	 }
  }
 }
 
 		 //�ڶ����������
//		 if(mission!=3){
//				if(xMHz<290){
//					fit=750000/(268.74-0.22*xMHz);		//ǿ�е������
//				}
//				else if(xMHz>290){
//					fit=750000/(153+0.2*xMHz);		//ǿ�����
//				}
//				else{
//					fit=4075;
//				}
//				if(fit>4095) fit=4095;		//����������ô��fit��Ϊ�������ֵ
//		}
//		 else{
//				if(SingleFreq<290){
//					fit=750000/(268.74-0.22*SingleFreq);		//ǿ�е������
//				}
//				else if(SingleFreq>290){
//					fit=750000/(153+0.2*SingleFreq);		//ǿ�����
//				}
//				else{
//					fit=4075;
//				}
//				if(fit>4095) fit=4095;		//����������ô��fit��Ϊ�������ֵ
//		 }
				//��һ���������
//			if(mission!=3){
//				if(xMHz<250){
//					fit=864000/(267-0.235*xMHz);		//ǿ�е������
//				}
//				else if(xMHz>290){
//					fit=864000/(146+0.233*xMHz);		//ǿ�����
//				}
//				else{
//					fit=4075;
//				}
//				if(fit>4095) fit=4095;		//����������ô��fit��Ϊ�������ֵ
//			}
//			else{
//				if(SingleFreq<250){
//					fit=864000/(267-0.235*SingleFreq);		//ǿ�е������
//				}
//				else if(SingleFreq>290){
//					fit=864000/(146+0.233*SingleFreq);		//ǿ�����
//				}
//				else{
//					fit=4075;
//				}
//				if(fit>4095) fit=4095;		//����������ô��fit��Ϊ�������ֵ
//			}
 