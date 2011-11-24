/***************************************************************************************
* 
* Copyright (c) 2008, 深圳市有方科技有限公司
* All rights reserved.
* 文件名称：neo_tcpip_demo.cpp
* 
* 摘    要：本代码仅供用户熟悉建立和发送TCP数据流程进行参考，代码中部分函数没有函数体
*           但是已经注明该函数的相应功能，需用户根据自己使用的硬件来添加，用户也可以
*			根据自己的需要对本例程进行部分调整与优化。
* 当前版本：1.0
* 
* 作    者：kevin
* 
* 参考资料：《有方GPRS模块_GPRS_PPP_TCP_详细流程.pdf》
* 
* 公司网址：http://www.neoway.com.cn
* 
* 模块系列：GRPS: M580/M580i/M580z/M590 (全球首款单芯片GPRS工业模块)
*
* 宣 传 语：有无线，方精彩！ Let's enjoy the wireless life!
* 
* 修改记录：
* 时间：20100416
* 版本：V1.1
* 内容：增加多链路的应用；
****************************************************************************************/

unsigned char gSendBuffer[256];
unsigned char gRecieveBuffer[256];


/***************************************************************************************
*函数名：TCPIP_CreateProcess
 
*描  述：创建TCPIP链接的完整过程，包括检查信号强度，CCID,网络注册状态，设置内部协议栈，
	     设置APN访问节点，设置PPP链接，设置TCP 服务器链接；
         请注意一个链路编号只能连接一个TCP连接或者UDP连接，不能连接多个连接。
*****************************************************************************************/ 
BOOL CTCPIPDlg::TCPIP_CreateProcess()
{
	///////////////////////////////////////////////////////////////////////////////
	// (1) 检查PBREADY;
	if( !CheckPBReady() ) 
	{// 模块上电后，会进行初始化，初始化完毕后，会送出+PBREADY;如果没有收到，请检查模块串口是否通；
		if( !CheckAT() ) 
			return FALSE;
		// 串口不通；请检查波特率是否一致，模块默认是115200；可以通过AT+IPR来设置波特率（只需要设置一次）
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// (2) 检查SIM卡；
	if( !CheckCCID() )
	{ 
		return FALSE;// 读取SIM卡失败；可能是没有插卡或者卡接触不良；
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// (3) 检查信号强度；
	if( !CheckCSQ() )
	{
		return FALSE;// 没有信号；
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// (4) 查询网络注册状态，如返回 +CREG: 0,1或+CREG: 0,5 则跳出循环，如超时则跳出;
	if( !CheckCreg() ) 
	{
		return FALSE;// 网络注册失败；
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// (5) 设置成内部协议栈
	SendATCommand("at+xisp=0\r",10);//发送"at+xisp=0
	
	TCPIP_CloseLink();// 规避假连接造成连接失败,可以不用判断返回值，这个一定要加上，请注意！！！
	
	///////////////////////////////////////////////////////////////////////////////
	// (6) 设置APN
	SendATCommand("at+cgdcont=1,IP,CMNET\r",22);//发送"at+cgdcont,请注意APN要根据各自的网络进行设置；
	
	///////////////////////////////////////////////////////////////////////////////
	// (7) 对于一些专网，如果需要用户认证指令，请加上这个步骤进行用户认证,	AT+XGAUTH=1,1,"GSM","1234";
	char buffer[30];
	memset(buffer,0,sizeof(buffer));
	strcpy(buffer,"AT+XGAUTH=1,1, GSM , 1234 ");
	buffer[14] = '"';
	buffer[18] = '"';
	buffer[20] = '"';
	buffer[25] = '"';
	buffer[26] = 0x0d;
	SendATCommand(buffer,27);
	
	///////////////////////////////////////////////////////////////////////////////
	// (8) 建立PPP连接
	if( !CreatePPP() )
	{
		return FALSE;// 建立PPP连接失败；
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// (9) 查询PPP链路状态
	if( !CheckPPPStatus() )
	{
		return FALSE;// 查询PPP连接失败；
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// (10) 建立TCP连接，注意这里的服务器地址是可变，要根据各自产品的情况进行修改；
	if( !CreateSingleTCPLink(0,"110.123.12.56","6800") )  
		return FALSE;
	
	return TRUE;
}

/****************************************************************************************
*函数名：SendATCommand

*描  述：发送指令函数；根据各自的平台情况做相应的修改；

*****************************************************************************************/ 
BOOL CTCPIPDlg::SendATCommand(char *strCmd,int len) 
{
	BOOL bRead;
	
	if( len<=0 )	return FALSE;
	
    // send to serial port
    bRead = csport.CSSend(gPortHandle,(unsigned char *)strCmd,len);

	return bRead;
}

/****************************************************************************************
*函数名：RecieveFromUart

*描  述：从串口中接收数据；根据各自的平台情况做相应的修改；

*****************************************************************************************/ 
int CTCPIPDlg::RecieveFromUart(unsigned char *gRecieveBuffer) 
{
	int nLength;

	memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
	csport.CSReceive(gPortHandle,gRecieveBuffer,&nLength);

	return nLength;
}

/****************************************************************************************
*函数名：Delay_ms

*描  述：延时函数，根据各自的平台修改一下里面的调用函数就可以了。

*****************************************************************************************/ 
void CTCPIPDlg::Delay_ms(int timelen) 
{
	Sleep(timelen);
}

/****************************************************************************************
*函数名：CheckCCID

*描  述：检查sim卡；

*****************************************************************************************/ 
BOOL CTCPIPDlg::CheckCCID()
{
    int timeout;  // 超时次数变量；

	timeout = 0;
	do
    {   
	    timeout++;

		SendATCommand("at+ccid\r",8); // 通过串口发送AT+CSQ回车
		
		Delay_ms(200);                 // 延时200ms；
		
		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
		RecieveFromUart(gRecieveBuffer); //获得返回值信息
		
		if( strstr((char *)gRecieveBuffer,"ERROR")==0 )
		{// 说明返回值不是ERROR,表示读取sim卡的CCID成功，退出；
			break;
		}
	}while(timeout<50);	  
	
	if( timeout>=50 )
	{	// 报错；
		return FALSE;// 读取SIM卡失败；可能是没有插卡或者卡接触不良；
	}

	return TRUE; 
}


/****************************************************************************************
*函数名：CheckPBReady

*描  述：检查是否收到+PBREADY；

*****************************************************************************************/ 
BOOL CTCPIPDlg::CheckPBReady()
{
    int timeout;  // 超时次数变量；

	timeout = 0;
	do
    {   
    	timeout++;
    	
		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
		RecieveFromUart(gRecieveBuffer); //获得返回值信息
		
		if( strstr((char *)gRecieveBuffer,"+PBREADY")>0 )
		{// 说明收到+PBREADY，退出；
			break;
		}
		Delay_ms(300);  // 延时300ms；
	}while(timeout<50);	  
	
	if( timeout>=50 )
	{	// 报错；
		return FALSE;// 没有收到；
	}

	return TRUE; 
}

/****************************************************************************************
*函数名：CheckAT

*描  述：检查信号强度；

*****************************************************************************************/ 
BOOL CTCPIPDlg::CheckAT()
{
    int timeout;  // 超时次数变量；

	timeout = 0;
	do
    {   
	    timeout++;

		SendATCommand("at\r",3); // 通过串口发送AT回车
		
		Delay_ms(100);                 // 延时100ms；
		
		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
		RecieveFromUart(gRecieveBuffer); //获得返回值信息
		
		if( strstr((char *)gRecieveBuffer,"OK")>0 )
		{// 说明串口通，退出；
			break;
		}
	}while(timeout<50);	  
	
	if( timeout>=50 )
	{	// 报错；
		return FALSE;
	}

	return TRUE; 
}

/***************************************************************************************
*函数名：CheckCSQ

*描  述：检查信号强度；

*****************************************************************************************/ 
BOOL CTCPIPDlg::CheckCSQ()
{
    int timeout;  // 超时次数变量；

	timeout = 0;
	do
    {   
	    timeout++;

		SendATCommand("at+csq\r",7); // 通过串口发送AT+CSQ回车
		
		Delay_ms(100);                 // 延时100ms；
		
		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
		RecieveFromUart(gRecieveBuffer); //获得返回值信息
		
		if( strstr((char *)gRecieveBuffer,"+CSQ:99,99")==0 )
		{// 说明检查的不是99，99，有信号，退出；
			break;
		}
	}while(timeout<50);	  
	
	if( timeout>=50 )
	{	// 报错；
		return FALSE;// 没有信号；
	}

	return TRUE; 
}

/****************************************************************************************
*函数名：CheckCreg

*描  述：检查网络注册状态；

*****************************************************************************************/ 
BOOL CTCPIPDlg::CheckCreg()
{
    int timeout;  // 超时次数变量；
    	
	timeout = 0;
	do
    {   
	    timeout++;

		SendATCommand("at+creg?\r",9); // 通过串口发送AT+CREG?回车
		
		Delay_ms(200);                 // 延时200ms；
		
		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
		RecieveFromUart(gRecieveBuffer); //获得返回值信息
		
		if( strstr((char *)gRecieveBuffer,"+CREG: 0,1")>0 ||  strstr((char *)gRecieveBuffer,"+CREG: 0,5")>0 )
		{// 如果返回值为+CREG: 0,1或+CREG: 0,5，则网络注册成功
			break;
		}
	}while(timeout<50);	  
	
	if( timeout>=50 )
	{// 报错；说明网络注册失败！
		return FALSE;// 网络注册失败；
	}
	
	return TRUE;
}

/****************************************************************************************
*函数名：CreatePPP

*描  述：建立PPP连接

*****************************************************************************************/ 
BOOL CTCPIPDlg::CreatePPP()
{
    int timeout;  // 超时次数变量；

	timeout = 0;
	do
	{  
		timeout++;
		
    	SendATCommand("at+xiic=1\r",10);// 发送"at+xiic=1
    	Delay_ms(200);                  // 延时100ms

		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
		RecieveFromUart(gRecieveBuffer); //获得返回值信息
		
		if( strstr((char *)gRecieveBuffer,"OK")>0 )
		{// 如返回OK 跳出循环
			break;
		}
	}while(timeout<50);	  
	
    if( timeout>=50 )
	{
		// 报错；说明PPP建立失败！
		return FALSE;
	}
	return TRUE;
}

/****************************************************************************************
*函数名：CheckPPPStatus

*描  述：查询PPP链路状态

*****************************************************************************************/ 
BOOL CTCPIPDlg::CheckPPPStatus()
{
	BOOL result_ok=FALSE;
    int value,timeout;  // 超时次数变量；
    	
	timeout=0;
	value = 0;
	do
	{   
		value++;
		
    	SendATCommand("at+xiic?\r",9);//发送"at+xiic?
    	
    	Delay_ms(500);   // 延时500ms;
		
		do
		{
			timeout++; 
    		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
    		RecieveFromUart(gRecieveBuffer); //获得返回值信息
    		
    		if( strstr((char *)gRecieveBuffer,"+XIIC:    1")>0 )
    		{//判断返回值 如果返回+XIIC:    1, *.*.*.*,则跳出循环
    			result_ok = TRUE;
    			break;
    		}
		}while(timeout<50);
		if( result_ok==TRUE )
			break;
	}while(value<3);	  

	if( timeout>=50 )
	{
		// 报错；说明PPP建立失败！
		return FALSE;
	}
	return TRUE;
}

/****************************************************************************************
*函数名：CreateTCPLink

*描  述：建立TCP连接；

*****************************************************************************************/ 
BOOL CTCPIPDlg::CreateTCPLink(char *linkStr,int size)
{
    int timeout;  // 超时次数变量；
	
	if( size<=0 ) 	return FALSE;
	
	timeout = 0;

	SendATCommand(linkStr,size);//发送AT指令建立TCP连接
		
	Delay_ms(1000);   // 延时1000ms;
	do
	{
		timeout++;
		Delay_ms(100);   // 延时10ms;
		
		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
		RecieveFromUart(gRecieveBuffer); //获得返回值信息
		if( strstr((char *)gRecieveBuffer,"+TCPSETUP:0,OK")>0 )
		{//判断返回值 如果返回+TCPSETUP:0,OK,则跳出循环
			break;
		}
	}while(timeout<50);
 	
	if( timeout>=50 )
	{
		// 报错；说明TCP链接失败！
		return FALSE;
	}
	
	return TRUE;
}


/****************************************************************************************
*函数名：TCPIP_CloseLink

*描  述：关闭TCP IP 链路；

*参  数：void
*****************************************************************************************/ 
void CTCPIPDlg::TCPIP_CloseLink()
{
	SendATCommand("at+tcpclose=0\r",14);// 发送AT+TCPCLOSE=0回车
	SendATCommand("at+tcpclose=1\r",14);// 发送AT+TCPCLOSE=1回车
}

/****************************************************************************************
*函数名：TCPIP_SendData

*描  述：发送TCP数据

*参  数：BOOL

*****************************************************************************************/
BOOL CTCPIPDlg::TCPIP_SendData()
{
	int timeout=0;

    while(1)
	{
		// 检查TCP buffer的大小，来判断是否发送数据；
		do
		{
			timeout++;
			SendATCommand("at+ipstatus=0\r",14);//发送 at+ipstatus=0回车查询TCP链路状态
			Delay_ms(100);
	
    		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
    		RecieveFromUart(gRecieveBuffer); //获得返回值信息
    		if( strstr((char *)gRecieveBuffer,"+IPSTATUS:0,CONNECT,TCP")>0 )
    		{// 如果返回+IPSTATUS:0,CONNECT,TCP,****，
    			break;
    		}
    		if( strstr((char *)gRecieveBuffer,"+IPSTATUS:0,DISCONNECT")>0 )
    		{
    			// 报错；说明链路没有建立；
    			return FALSE;
    		}
   		}while(timeout<50);
   		
		if( timeout>=50 )
		{
			// 报错；说明TCP链接失败！
			return FALSE;
		}

		// 如果TCP状态OK 则开始发送TCP指令；
		SendATCommand("at+tcpsend=0,50\r",16);	//发送 at+tcpsend=0,50回车，发送50个字节；

		// 判断接收发送符：">"
		do
		{
			timeout++;
    		Delay_ms(100);   // 延时100ms;
    		
    		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
    		RecieveFromUart(gRecieveBuffer); //获得返回值信息
    		if( strstr((char *)gRecieveBuffer,">")>0 )
    		{//判断返回值 如果返回>,则跳出循环，表示可以发送数据了；
    			break;
    		}
   		}while(timeout<50);
   		
   		if( timeout>=50 )
		{
			// 报错；说明没有收到 >,不能发送数据！
			return FALSE;
		}

		strcpy((char *)gSendBuffer,"12345678901234567890123456789012345678901234567890\r");
		// 开始发送数据；发送数据时，必须在数据包的末尾加上0x0d 作为结束符，但该结束符不算到数据长度里，
		SendATCommand((char *)gSendBuffer,50+1); //发送50个字节数据和回车；记住数据最后要加回车符；
		Delay_ms(100);   // 延时100ms;
		
		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
		RecieveFromUart(gRecieveBuffer); //获得返回值信息
		if( strstr((char *)gRecieveBuffer,"+TCPSEND:0,50")>0 )
		{// 判断返回值 如果返回+TCPSEND:0,50;表示发送成功；
			return TRUE;
		}
		else if( strstr((char *)gRecieveBuffer,"+TCPSEND:Error")>0 )
		{// 判断返回值 如果返回+TCPSEND:Error;在链路0 上发送50字节的数据，该链路尚未建立，发送失败。
			return FALSE;
		}
		else if( strstr((char *)gRecieveBuffer,"+TCPSEND:Buffer not enough")>0 )
		{// 判断返回值 如果返回+TCPSEND:Buffer not enough;在链路0 上发送50字节的数据，内部buffer不足，发送失败。
			return FALSE;
		}
		
		// 根据情况加上相应的代码进行处理  ....
	}
}

/****************************************************************************************
*函数名：OnTcpipTest

*描  述：TCP-IP 链路创建和数据发送测试函数

*****************************************************************************************/
void CTCPIPDlg::OnTcpipTest() 
{
	// TODO: Add your control notification handler code here
	OpenPortMode();  // 打开串口；
	
	TCPIP_CreateProcess(); 
	TCPIP_SendData();
	TCPIP_CloseLink(); // 关闭链路；
	
	ClosePortMode(); // 关闭串口；
}

/****************************************************************************************
*函数名：TCPIP_SendData

*描  述：接收TCP数据，有数据来时，通过串口中断触发，对数据格式进行判断，
	     如果是+TCPRECV:0开头的，那说明接收到TCP数据；
		 如：+TCPRECV:0,10,1234567890\r\n
	
		 请根据各自平台的特点对数据接收进行相应的处理；这个函数仅仅供参考。
	
		 有中断触发时，调用这个函数TCPIP_RecieveData进行接收；
*参  数：BOOL

*****************************************************************************************/
BOOL CTCPIPDlg::TCPIP_RecieveData()
{
	int timeout=0;

	do
	{
		timeout++;
		Delay_ms(100);

		memset(gRecieveBuffer,0x00,sizeof(gRecieveBuffer));
		RecieveFromUart(gRecieveBuffer); //获得返回值信息
		if( strstr((char *)gRecieveBuffer,"+TCPRECV:0")>0 )
		{// 如果返回+TCPRECV:0
			// 解析后面的长度和数据；在这里加上代码；
			// ...
			break;
		}
	}while(timeout<50);
	
	if( timeout>=50 )
	{
		// 报错；
		return FALSE;
	}

	return TRUE;
}

/****************************************************************************************
*函数名：CreateSingleTCPLink

*描  述：创建单TCP链路.
	
*参  数：BOOL

*****************************************************************************************/
BOOL CTCPIPDlg::CreateSingleTCPLink(unsigned char iLinkNum,char *strServerIP,char *strPort)
{
	int length;
	char buffer[50];
	
	memset(buffer,0x00,sizeof(buffer));
	sprintf(buffer,"AT+TCPSETUP=%d,%s,%s\r",iLinkNum,strServerIP,strPort);
	length = strlen((char *)buffer);
	if( !CreateTCPLink(buffer,length) )  
	{
		return FALSE;// 建立TCP连接失败；
	}
	return TRUE;
}

typedef enum
{
	NEO_LINK0,
	NEO_LINK1,
	NEO_LINK2,
	NEO_LINK3
}neo_link_enum_type;
	
/****************************************************************************************
*函数名：CreateMultiTCPLink

*描  述：创建多TCP链路,创建完毕后，每个链路的数据收发要根据链路号来区别。
	
	     多链路的版本，AT指令没有变化，只是链路编号范围扩大了，0-4；
	     
	     注意：关闭链路时，一定要把所有创建的链路都关闭。
	     	 
*参  数：BOOL

*****************************************************************************************/
BOOL CTCPIPDlg::CreateMultiTCPLink()
{
	// 创建第一路TCP链路；链路号是0；
	if( !CreateSingleTCPLink(NEO_LINK0,"110.123.12.56","6800") )  
		return FALSE;

	// 创建第二路TCP链路；链路号是1；
	if( !CreateSingleTCPLink(NEO_LINK1,"110.122.12.55","6800") )  
		return FALSE;

	// 创建第三路TCP链路；链路号是2；
	if( !CreateSingleTCPLink(NEO_LINK2,"110.121.12.54","6800") )  
		return FALSE;
	
	return TRUE;
}

/****************************************************************************************
*函数名：TCPIP_CloseMulitLink

*描  述：关闭TCP IP 链路；

*参  数：void
*****************************************************************************************/ 
void CTCPIPDlg::TCPIP_CloseMulitLink()
{
	SendATCommand("AT+TCPCLOSE=0\r",14);// 发送AT+TCPCLOSE=0回车,关闭链路0，
	SendATCommand("AT+TCPCLOSE=1\r",14);// 发送AT+TCPCLOSE=1回车,关闭链路1，
	SendATCommand("AT+TCPCLOSE=2\r",14);// 发送AT+TCPCLOSE=1回车,关闭链路2，
}


//////////////////////////////////////////////End//////////////////////////////////////////