/**
* rs485.c -- RS485驱动接口头文件
* 
* 
* 创建时间: 2010-4-24
* 最后修改时间: 2010-4-27
*/

#ifndef _RS485_H
#define _RS485_H

//RS485 端口数
#define RS485_PORTNUM	3

/**
* @brief 复位RS485端口
* @param port RS485端口号
* @return 0成功, 否则失败
*/
int Rs485Reset(unsigned int port);
/**
* @brief 设置RS485端口波特率和属性
* @param port 端口号
* @param baud 波特率
* @param databits 数据位, 5~8
* @param stopbits 停止位, 1~2
* @param parity 校验位
*/
void Rs485Set(unsigned int port, int baud, int databits, int stopbits, char parity);
/**
* @brief 向RS485端口发送数据
* @param port 端口号
* @param buf 发送缓存区
* @param len 缓存区长度
* @param 0成功, 否则失败
*/
int Rs485Send(unsigned int port, const unsigned char *buf, int len);
/**
* @brief 从RS485端口接收数据
* @param port 端口号
* @param buf 接收缓存区
* @param len 缓存区长度
* @return 失败返回-1, 成功返回接收到的字节数, 返回0表示未接收到数据
*/
int Rs485Recv(unsigned int port, unsigned char *buf, int len);

/**
* @brief 锁住Rs485端口
* @param port 端口号
*/
void Rs485Lock(unsigned int port);
/**
* @brief 解锁Rs485端口
* @param port 端口号
*/
void Rs485Unlock(unsigned int port);

#endif /*_RS485_H*/
