/**
* grpbin.h -- 二进制群组文件储存头文件
* 
* 
* 创建时间: 2010-5-6
* 最后修改时间: 2010-5-6
*/

#ifndef _STORAGE_GRPBIN_H
#define _STORAGE_GRPBIN_H

typedef struct {
	char *pfile;
	int itemlen;  //项目长度
	int itemnum;  //项目数目
	unsigned short crctemp;
	char flag;
} grpbin_ref_t;

/**
* @brief 打开一个群BIN文件
* @param file 文件名
* @param magic 文件标识字
* @param flag 打开方式. 'r':读取方式; 'w':写方式
* @param pref 文件索引变量指针
* @return 成功返回0,否则失败
*/
int OpenGrpBinFile(const char *file, unsigned long magic, char flag, grpbin_ref_t *pref);
/**
* @brief 从群BIN文件中读取一个项目
* @param pref 文件索引变量指针
* @param buffer 缓存区指针
* @param len 缓存区长度
* @return 成功返回实际读取长度,失败返回-1
*/
int ReadGrpBinFileItem(grpbin_ref_t *pref, unsigned char *buffer, int len);
/**
* @brief 在群BIN文件中写入一个项目
* @param pref 文件索引变量指针
* @param buffer 缓存区指针
* @param len 缓存区长度
* @return 成功返回0,否则失败
*/
int WriteGrpBinFileItem(grpbin_ref_t *pref, unsigned char *buffer);
/**
* @brief 关闭一个群BIN文件
* @param pref 文件索引变量指针
*/
void CloseGrpBinFile(grpbin_ref_t *pref);

#endif /*#ifndef _STORAGE_GRPBIN_H*/

