/**
* bin.c -- 二进制群组文件储存
* 
* 
* 创建时间: 2010-5-6
* 最后修改时间: 2010-5-6
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/sys/grpbin.h"
#include "include/lib/crc.h"
#include "include/debug.h"

typedef struct {
	unsigned short headcrc;
	unsigned short reserv;
	unsigned long magic;
	unsigned short itemlen;
	unsigned short itemnum;
} grpbin_filehead_t;

/**
* @brief 打开一个群BIN文件
* @param file 文件名
* @param magic 文件标识字
* @param flag 打开方式. 'r':读取方式; 'w':写方式
* @param pref 文件索引变量指针
* @return 成功返回0,否则失败
*/
int OpenGrpBinFile(const char *file, unsigned long magic, char flag, grpbin_ref_t *pref)
{
	grpbin_filehead_t head;
	FILE *pf;
	unsigned short crc;

	AssertLogReturn(NULL==file, 1, "null file\n");
	AssertLogReturn(NULL==pref, 1, "null ref\n");

	pref->flag = flag;

	if('r' == flag) {
		unsigned char cache[64];
		unsigned short temp;
		int readlen, rtn, offset;

		pf = fopen(file, "rb");
		if(NULL == pf) return 1;

		if(fread(&head, sizeof(head), 1, pf) <= 0) {
			ErrorLog("%s file head too short\n", file);
			goto mark_fail;
		}

		if(head.magic != magic) {
			ErrorLog("%s magic invalid(0x%08X)\n", file, head.magic);
			goto mark_fail;
		}

		if(head.itemlen&0x01) {
			ErrorLog("%s invalid itemlen(%d)\n", file, head.itemlen);
			goto mark_fail;
		}

		crc = CalculateCRC((unsigned char *)&(head.reserv), sizeof(head)-2);
		if(head.headcrc != crc) {
			ErrorLog("%s head crc error(0x%04X, should be 0x%04X)\n", file, head.headcrc, crc);
			goto mark_fail;
		}

		temp = crc = 0;
		readlen = offset =0;
		while(1) {
			rtn = fread(&cache[offset], 1, 32, pf);
			if(rtn < 32) {  //end
				if(rtn < 2) {  //no data
					if(readlen == 0) {
						ErrorLog("%s no end\n", file);
						goto mark_fail;
					}

					temp = crc;
					offset = (offset) ? 0 : 32;
					CalculateCRCStep(&cache[offset], 30, &temp);
					crc = ((unsigned short)cache[offset+31]<<8) + ((unsigned short)cache[offset+30]);
				}
				else {
					CalculateCRCStep(&cache[offset], rtn-2, &temp);
					crc = ((unsigned short)cache[offset+rtn-1]<<8) + ((unsigned short)cache[offset+rtn-2]);
				}

				if(crc != temp) {
					ErrorLog("%s tail crc error(0x%04X, should be 0x%04X)\n", file, crc, temp);
					goto mark_fail;
				}

				fseek(pf, sizeof(grpbin_filehead_t), SEEK_SET);
				break;
			}
			else {
				crc = temp;
				CalculateCRCStep(&cache[offset], 32, &temp);
				offset = (offset) ? 0 : 32;
				readlen += 32;
			}
		}

		pref->itemlen = (int)head.itemlen & 0xffff;
		pref->itemnum = (int)head.itemnum & 0xffff;
		pref->pfile = (char *)pf;
	}
	else if('w' == flag) {
		//printf("open %s\n", file);
		if(pref->itemlen <= 0 || pref->itemlen > 0xffff || (pref->itemlen&0x01)) {
			ErrorLog("invalid itemlen(%d)\n", pref->itemlen);
			return 1;
		}
		if(pref->itemnum <= 0 || pref->itemnum > 0xffff) {
			ErrorLog("invalid itemnum(%d)\n", pref->itemnum);
			return 1;
		}

		remove(file);
		pf = fopen(file, "wb");
		if(NULL == pf) {
			ErrorLog("can not open %s for write\n", file);
			return 1;
		}

		pref->pfile = (char *)pf;
		pref->crctemp = 0;

		head.magic = magic;
		head.itemlen = pref->itemlen;
		head.itemnum = pref->itemnum;
		head.reserv = 0;
		head.headcrc = CalculateCRC((unsigned char *)&(head.reserv), sizeof(head)-2);
		fwrite(&head, sizeof(head), 1, pf);
	}

	return 0;

mark_fail:
	fclose(pf);
	return 1;
}

/**
* @brief 从群BIN文件中读取一个项目
* @param pref 文件索引变量指针
* @param buffer 缓存区指针
* @param len 缓存区长度
* @return 成功返回实际读取长度,失败返回-1
*/
int ReadGrpBinFileItem(grpbin_ref_t *pref, unsigned char *buffer, int len)
{
	FILE *pf;
	unsigned short crc1, crc2;

	AssertLogReturn(NULL==pref, 1, "null ref\n");

	if(pref->itemlen > len) {
		ErrorLog("buffer too short(%d)\n", len);
		return -1;
	}

	pf = (FILE *)pref->pfile;
	while(1) {
		if(fread(&crc1, sizeof(crc1), 1, pf) <= 0) return -1;
		if(fread(buffer, pref->itemlen, 1, pf) <= 0) return -1;

		crc2 = CalculateCRC(buffer, pref->itemlen);
		if(crc1 == crc2) return pref->itemlen;
	}

	return -1;
}

/**
* @brief 在群BIN文件中写入一个项目
* @param pref 文件索引变量指针
* @param buffer 缓存区指针
* @param len 缓存区长度
* @return 成功返回0,否则失败
*/
int WriteGrpBinFileItem(grpbin_ref_t *pref, unsigned char *buffer)
{
	FILE *pf;
	unsigned short crc;

	AssertLogReturn(NULL==pref, 1, "null ref\n");

	pf = (FILE *)pref->pfile;
	crc = CalculateCRC(buffer, pref->itemlen);
	CalculateCRCStep((unsigned char *)&crc, 2, &pref->crctemp);
	CalculateCRCStep(buffer, pref->itemlen, &pref->crctemp);

	fwrite(&crc, sizeof(crc), 1, pf);
	fwrite(buffer, pref->itemlen, 1, pf);

	return 0;
}

/**
* @brief 关闭一个群BIN文件
* @param pref 文件索引变量指针
*/
void CloseGrpBinFile(grpbin_ref_t *pref)
{
	FILE *pf;

	if(NULL == pref) {
		ErrorLog("null ref\n");
		return;
	}

	pf = (FILE *)pref->pfile;

	if(pref->flag == 'w') fwrite(&pref->crctemp, 2, 1, pf);

	fclose(pf);
}
