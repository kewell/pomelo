/**
* align.h -- 数据对齐
* 
* 
* 创建时间: 2010-6-11
* 最后修改时间: 2010-6-11
*/

#ifndef _LIB_ALIGN_H
#define _LIB_ALIGN_H

#if 1
typedef union {
	unsigned char uc[4];
	unsigned int ul;
} varul_t;

typedef union {
	unsigned char uc[2];
	unsigned short us;
} varus_t;

#define MAKE_SHORT(buf) ({ \
	varus_t _tmp_varus; \
	_tmp_varus.uc[0] = ((unsigned char *)(buf))[0]; \
	_tmp_varus.uc[1] = ((unsigned char *)(buf))[1]; \
	_tmp_varus.us; \
})

#define DEPART_SHORT(usv, buf) { \
	varus_t _tmp_varus; \
	_tmp_varus.us = usv; \
	((unsigned char *)(buf))[0] = _tmp_varus.uc[0]; \
	((unsigned char *)(buf))[1] = _tmp_varus.uc[1]; \
}

#define MAKE_LONG(buf) ({ \
	varul_t _tmp_varul; \
	_tmp_varul.uc[0] = ((unsigned char *)(buf))[0]; \
	_tmp_varul.uc[1] = ((unsigned char *)(buf))[1]; \
	_tmp_varul.uc[2] = ((unsigned char *)(buf))[2]; \
	_tmp_varul.uc[3] = ((unsigned char *)(buf))[3]; \
	_tmp_varul.ul; \
})

#define DEPART_LONG(ulv, buf) { \
	varul_t _tmp_varul; \
	_tmp_varul.ul = ulv; \
	((unsigned char *)(buf))[0] = _tmp_varul.uc[0]; \
	((unsigned char *)(buf))[1] = _tmp_varul.uc[1]; \
	((unsigned char *)(buf))[2] = _tmp_varul.uc[2]; \
	((unsigned char *)(buf))[3] = _tmp_varul.uc[3]; \
}

#else

#define MAKE_SHORT(buf)		( \
	((unsigned short)(((unsigned char *)(buf))[0])) + \
	((unsigned short)(((unsigned char *)(buf))[1]) << 8))

#define DEPART_SHORT(us, buf)	{ \
	((unsigned char *)(buf))[0] = (unsigned char)(us); \
	((unsigned char *)(buf))[1] = (unsigned char)((unsigned short)(us)>>8); \
}

#define MAKE_LONG(buf)		( \
	((unsigned int)(((unsigned char *)(buf))[0])) + \
	((unsigned int)(((unsigned char *)(buf))[1]) << 8) + \
	((unsigned int)(((unsigned char *)(buf))[2]) << 16) + \
	((unsigned int)(((unsigned char *)(buf))[3]) << 24))

#define DEPART_LONG(ul, buf)	{ \
	((unsigned char *)(buf))[0] = (unsigned char)(ul); \
	((unsigned char *)(buf))[1] = (unsigned char)((unsigned int)(ul)>>8); \
	((unsigned char *)(buf))[2] = (unsigned char)((unsigned int)(ul)>>16); \
	((unsigned char *)(buf))[3] = (unsigned char)((unsigned int)(ul)>>24); \
}

#endif

#endif /*_LIB_ALIGN_H*/

