/**
* sndtime.h -- 数据任务发送时间处理
* 
* 
* 创建时间: 2010-5-27
* 最后修改时间: 2010-5-27
*/

#ifndef _MDB_SENDTIME_H
#define _MDB_SENDTIME_H

unsigned int GetSndTime(const unsigned char *pnfn);
void ClearSndTime(void);
void SetSndTime(const unsigned char *pnfn, unsigned int sndtime, unsigned char step);
void SaveSndTime(void);
void SaveSndTimeFile(void);

void RemoveDataTaskTime(int taskid);
void InitDataTaskTime(int taskid);

#endif /*_MDB_SENDTIME_H*/

