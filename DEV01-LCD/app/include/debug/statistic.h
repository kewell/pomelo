/**
* statistic.h -- 统计量接口
* 
* 
* 创建时间: 2010-7-16
* 最后修改时间: 2010-7-16
*/

#ifndef _DEBUG_STATISTIC_H
#define _DEBUG_STATISTIC_H

struct debug_statistic_t {
	int count;
	int group;
	const char *name;
};

//声明一个统计变量
#define DECLARE_STATISTIC(group, name, initval) \
	struct debug_statistic_t _statistic_##name \
	__attribute__((__used__)) \
	__attribute__((section("_statistic"), unused)) \
	= {initval, group, #name}

//引用一个统计变量
#define REF_STATICSTIC(name)			extern debug_statistic_t _statistic_##name

//统计变量计数加addval
#define ADD_STATISTIC(name, addval)		_statistic_##name.count += addval
//统计变量计数加1
#define ADDONE_STATISTIC(name)			_statistic_##name.count += 1
//设置统计变量计数为val
#define SET_STATISTIC(name, val)		_statistic_##name.count = val
//得到统计变量计数
#define GET_STATISTIC(name)				_statistic_##name.count

#define STATISGROUP_SYS			1
#define STATISGROUP_UPLINK		2
#define STATISGROUP_DOWNLINK	3
#define STATISGROUP_MAX			STATISGROUP_DOWNLINK
#define STATISGROUP_NOSAVE		10

#endif /*_DEBUG_STATISTIC_H*/

