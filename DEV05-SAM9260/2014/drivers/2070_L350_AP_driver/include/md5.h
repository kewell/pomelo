/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/


/****************************************************************************
    Module Name:
    MD5

    Abstract:
    RFC1321: The MD5 Message-Digest Algorithm
    
    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
    Eddy        2008/11/24      Create md5
***************************************************************************/

#ifndef __MD5_H__
#define __MD5_H__

#ifdef ALGORITHM_TEST
#include "alg_test.h"
#else
#include "rt_config.h"
#endif /* ALGORITHM_TEST */

/* Algorithm options */
#define MD5_SUPPORT

#ifdef MD5_SUPPORT
#define MD5_BLOCK_SIZE    64 /* 512 bits = 64 bytes */
#define MD5_DIGEST_SIZE   16 /* 128 bits = 16 bytes */
typedef struct {
    UINT32 HashValue[4];
    UINT64 MessageLen;
    UINT8  Block[MD5_BLOCK_SIZE];
    UINT8  BlockLen;
} MD5_CTX_STRUC, *PMD5_CTX_STRUC;

VOID MD5_Init (
    IN  MD5_CTX_STRUC *pMD5_CTX);
VOID MD5_Hash (
    IN  MD5_CTX_STRUC *pMD5_CTX);
VOID MD5_Append (
    IN  MD5_CTX_STRUC *pMD5_CTX, 
    IN  const UINT8 Message[], 
    IN  UINT MessageLen);
VOID MD5_End (
    IN  MD5_CTX_STRUC *pMD5_CTX, 
    OUT UINT8 DigestMessage[]);
VOID MD5 (
    IN  const UINT8 Message[], 
    IN  UINT MessageLen,
    OUT UINT8 DigestMessage[]);
#endif /* MD5_SUPPORT */

#endif /* __SHA2_H__ */

