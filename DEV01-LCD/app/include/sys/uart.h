/**
* uart.h -- UART驱动接口头文件
* 
* 
* 创建时间: 2010-4-23
* 最后修改时间: 2010-4-23
*/

#ifndef _UART_H
#define _UART_H

//#define UART_PORTNUM		6
#define UART_PORTNUM		7

/**
* @brief 打开一个UART端口
* @param port 端口号, 0~4
* @return 0成功, 否则失败
*/
int UartOpen(unsigned int port);
/**
* @brief 关闭一个已打开的UART端口
* @param port 端口号, 0~4
*/
void UartClose(unsigned int port);

/**
* @brief 设置UART端口波特率
* @param port 端口号, 0~4
* @param baud 波特率
*/
void UartSetBaudrate(unsigned int port, int baud);
/**
* @brief 设置UART端口属性
* @param port 端口号, 0~4
* @param databits 数据位, 5~8
* @param stopbits 停止位, 1~2
* @param parity 校验位
*/
void UartSetParity(unsigned int port, int databits,int stopbits, char parity);
/**
* @brief 设置UART端口波特率和属性
* @param port 端口号, 0~4
* @param baud 波特率
* @param databits 数据位, 5~8
* @param stopbits 停止位, 1~2
* @param parity 校验位
*/
void UartSet(unsigned int port, int baud, int databits, int stopbits, char parity);

/**
* @brief 向UART端口发送数据
* @param port 端口号, 0~4
* @param buf 发送缓存区
* @param len 缓存区长度
* @param 0成功, 否则失败
*/
int UartSend(unsigned int port, const unsigned char *buf, int len);
/**
* @brief 从UART端口接收数据
* @param port 端口号, 0~4
* @param buf 接收缓存区
* @param len 缓存区长度
* @return 失败返回-1, 成功返回接收到的字节数, 返回0表示未接收到数据
*/
int UartRecv(unsigned int port, unsigned char *buf, int len);

/**
* @brief 获取UART端口的文件号(ppp使用)
* @param port 端口号, 0~4
* @return 失败返回-1, 成功返回文件号
*/
int UartGetFid(unsigned int port);

#endif /*_UART_H*/
