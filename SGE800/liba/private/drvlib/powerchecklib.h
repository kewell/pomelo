/******************************************************************************
 * ���̵����ɷ����޹�˾                                    ��Ȩ��2008-2015    *
 ******************************************************************************
 * ��Դ���뼰������ĵ�Ϊ�����������̵����ɷ����޹�˾�������У�δ��������     *
 * �ɲ��������޸Ļ򷢲�������׷����صķ������Ρ�                           *
 *                                                                            *
 *                       �����������̹ɷ����޹�˾                             *
 *                       www.xjgc.com                                         *
 *                       (0374) 321 2924                                      *
 *                                                                            *
 ******************************************************************************
 * 
 * ��Ŀ����		:	�����ϵ������������ͷ�ļ�
 * �ļ���		:	powrdlib.h
 * ����			:	Ӧ�ó������õ���ioctl����������
 * �汾			:	1.0.1
 * ����			:	·ȽȽ
 *
 ******************************************************************************/
	
#ifndef _POWERCHECKLIB_H
#define _POWERCHECKLIB_H


#define PWRD_IOC_MAGIC 0xEF			//ioctl cmd����

#define PWRD_SET_IO	_IO(PWRD_IOC_MAGIC, 2)	//����Ҫ��ص�io
#define PWRD_TIMEOUT	_IO(PWRD_IOC_MAGIC, 3)	//���ü�س�ʱʱ�䣬��λjiffies
#define PWRD_SET_MODE	_IO(PWRD_IOC_MAGIC, 4)	//���ü��ģʽ�����磬�ϵ磬���ϵ�

#endif  /* _POWERCHECKLIB_H */