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
    HMAC

    Abstract:
    FIPS 198: The Keyed-Hash Message Authentication Code (HMAC)
    
    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
    Eddy        2008/11/24      Create HMAC-SHA1, HMAC-SHA256
***************************************************************************/
#ifndef __HMAC_H__
#define __HMAC_H__

#ifdef ALGORITHM_TEST
#include "alg_test.h"
#else
#include "rt_config.h"
#endif /* ALGORITHM_TEST */

#ifdef SHA1_SUPPORT
#define HMAC_SHA1_SUPPORT
VOID HMAC_SHA1 (
    IN  const UINT8 Key[], 
    IN  UINT KeyLen, 
    IN  const UINT8 Message[], 
    IN  UINT MessageLen, 
    OUT UINT8 MAC[],
    IN  UINT MACLen);
#endif /* SHA1_SUPPORT */

#ifdef SHA256_SUPPORT
#define HMAC_SHA256_SUPPORT
VOID HMAC_SHA256 (
    IN  const UINT8 Key[], 
    IN  UINT KeyLen, 
    IN  const UINT8 Message[], 
    IN  UINT MessageLen, 
    OUT UINT8 MAC[],
    IN  UINT MACLen);
#endif /* SHA256_SUPPORT */

#ifdef MD5_SUPPORT
#define HMAC_MD5_SUPPORT
VOID HMAC_MD5 (
    IN  const UINT8 Key[], 
    IN  UINT KeyLen, 
    IN  const UINT8 Message[], 
    IN  UINT MessageLen, 
    OUT UINT8 MAC[],
    IN  UINT MACLen);
#endif /* MD5_SUPPORT */


/******************************************************************************/
// TODO: Albert, the following code should move to other location.
#ifndef	uint8
#define	uint8  unsigned	char
#endif

#ifndef	uint32
#define	uint32 unsigned	int
#endif

#ifndef _AES_H
#define _AES_H

typedef	struct
{
    UINT32 erk[64];     /* encryption round keys */
    UINT32 drk[64];     /* decryption round keys */
    int nr;             /* number of rounds */
}
aes_context;

int	 rtmp_aes_set_key( aes_context *ctx,    uint8 *key,	int	nbits );
void rtmp_aes_encrypt( aes_context *ctx,    uint8 input[16], uint8 output[16] );
void rtmp_aes_decrypt( aes_context *ctx,    uint8 input[16], uint8 output[16] );

void F(char *password, unsigned char *ssid, int ssidlength, int iterations, int count, unsigned char *output);
int PasswordHash(char *password, unsigned char *ssid, int ssidlength, unsigned char *output);
void kd_hmac_sha256(	
    unsigned char 	*key, 
    unsigned int 	key_len,
    unsigned char 	*text, 
	unsigned int 	text_len,
    unsigned char 	*output, 
    unsigned int 	output_len);

#endif /* aes.h */


#endif /* __HMAC_H__ */
