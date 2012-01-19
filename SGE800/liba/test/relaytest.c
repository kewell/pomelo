/*****************************************************************************/
/*���̵����ɷ����޹�˾                                     ��Ȩ��2008-2015   */
/*****************************************************************************/
/* ��Դ���뼰������ĵ�Ϊ�����������̵����ɷ����޹�˾�������У�δ��������    */
/* �ɲ��������޸Ļ򷢲�������׷����صķ������Ρ�                          */
/*                                                                           */
/*                      �����������̹ɷ����޹�˾                             */
/*                      www.xjgc.com                                         */
/*                      (0374) 321 2924                                      */
/*                                                                           */
/*****************************************************************************/


/******************************************************************************
    ��Ŀ����    ��  SGE800���������ն�ƽ̨
    �ļ���      ��  relaytest.c
    ����        ��  ���ļ����ڵ��ԺͲ���ƽ̨��adc
    �汾        ��  0.1
    ����        ��  ·ȽȽ
    ��������    ��  2010.04
******************************************************************************/


//C��ͷ�ļ�
#include <stdio.h>		//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep

//ƽ̨��ͷ�ļ�
#include "../include/relay.h"
void p_err(int exp)
{
	//_TestCount ++;
	if (exp != 0)
	{
	//	_ErrorCount ++;
//		printf(msg);
		switch(exp)
		{
			case -1:
				printf(" error: ERR_SYS\n");
				break;
			case -2:
				printf(" error: ERR_NODEV/ERR_NOFILE\n");
				break;
			case -3:
				printf(" error: ERR_TIMEOUT\n");
				break;
			case -4:
				printf(" error: ERR_INVAL\n");
				break;
			case -5:
				printf(" error: ERR_NOFUN\n");
				break;
			case -6:
				printf(" error: ERR_BUSY\n");
				break;
			case -7:
				printf(" error: ERR_NOINIT\n");
				break;
			case -8:
				printf(" error: ERR_NOMEM\n");
				break;
			case -9:
				printf(" error: ERR_NODISK\n");
				break;
			case -10:
				printf(" error: ERR_NORECORD\n");
				break;
			case -11:
				printf(" error: ERR_CFG\n");
				break;
			case -12:
				printf(" error: ERR_NOCFG\n");
				break;
			case -13:
				printf(" error: ERR_DEVUNSTAB\n");
				break;
			case -14:
				printf(" error: ERR_DISCONNECT\n");
				break;
			case -80:
				printf(" error: ERR_OTHER\n");
				break;
			default:
				printf("\n");
				break;
		}

	}
}
int main()
{
	int ret;	
	char func;
	int pin;
	u32 delay,last,period;

	ret = relay_init();
	if(ret < 0 ){
		p_err(ret);
		goto error;
	}
	printf("+----------------------------------------+\n");
	printf("| 	 ARM  Linux  relay lib test		|\n");
	printf("| 	  Write on 2010.04.13 ROY		|\n");
	printf("+----------------------------------------+\n"); 
help:
	printf("\n Please select the function to test ( Main Menu):\n ");
	printf("\n  h: help ");
	printf("\n  a: active relay");
	printf("\n  d: deactive relay");
	printf("\n  g: get relay status");
	printf("\n  c: close relay modoul");
	printf("\n ");

	while(1){
	 printf("please select function ");
	 scanf("%s",&func);
	 printf("\n ");
	 switch(func)
	 {
		 case 'h':	//�����Գ������   
			goto help;
			break;			
		case 'a':	//�����Գ��򣬼̵�������
			printf("please input id 0~7 \n");
			scanf("%d",&pin);
			printf("please input delay \n");
			scanf("%u",&delay);
			printf("please input last \n");
			scanf("%u",&last);
			printf("please input period \n");
			scanf("%u",&period);
			ret = relay_on(pin,delay,last,period);
			if(ret < 0 ){
				p_err(ret);
				goto help;
			}
			printf("active relay done\n");
			break;		
		case 'd':		//�̵���������
			printf("please input id \n");
			scanf("%d",&pin);

			ret = relay_off(pin);
			if(ret < 0 ){
				p_err(ret);
				goto help;
			}
			break;
		case 'g':	//��ȡ�̵���״̬ 1-������0-������
			printf("please input id \n");
			scanf("%d",&pin);
			ret = relay_check(pin);
			if(ret < 0 ){
				p_err(ret);
				goto help;
			}
			printf("relay status is %d\n",ret);
			break;
		case 'c':
			relay_close();
			exit(1);						   
			break;	
		
		default: 
			relay_close();
		 	exit(1); 
		}
	} 
	   

error:
	printf ("system error\n");
	relay_close();
	return 0;
}
