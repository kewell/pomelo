#include <stdio.h>
#include <stdlib.h>
#include "md5.h"

static void Transform (UINT4 *, UINT4 *);

static unsigned char PADDING[64] = {
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* F, G, H and I are basic MD5 functions */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s, ac) \
  {(a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) \
  {(a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) \
  {(a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) \
  {(a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }

#define UL(x)	(x)

/* The routine MD5Init initializes the message-digest context
   mdContext. All fields are set to zero.
 */
void MD5Init (MD5_CTX *mdContext)
{
  mdContext->i[0] = mdContext->i[1] = (UINT4)0;

  /* Load magic initialization constants.
   */
  mdContext->buf[0] = (UINT4)0x67452301;
  mdContext->buf[1] = (UINT4)0xefcdab89;
  mdContext->buf[2] = (UINT4)0x98badcfe;
  mdContext->buf[3] = (UINT4)0x10325476;
}


/* The routine MD5Update updates the message-digest context to
   account for the presence of each of the characters inBuf[0..inLen-1]
   in the message whose digest is being computed.
 */
void MD5Update (MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen)
{
  UINT4 in[16];
  int mdi;
  unsigned int i, ii;

  /* compute number of bytes mod 64 */
  mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

  /* update number of bits */
  if ((mdContext->i[0] + ((UINT4)inLen << 3)) < mdContext->i[0])
    mdContext->i[1]++;
  mdContext->i[0] += ((UINT4)inLen << 3);
  mdContext->i[1] += ((UINT4)inLen >> 29);

  while (inLen--) {
    /* add new character to buffer, increment mdi */
    mdContext->in[mdi++] = *inBuf++;

    /* transform if necessary */
    if (mdi == 0x40) {
      for (i = 0, ii = 0; i < 16; i++, ii += 4)
        in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
                (((UINT4)mdContext->in[ii+2]) << 16) |
                (((UINT4)mdContext->in[ii+1]) << 8) |
                ((UINT4)mdContext->in[ii]);
      Transform (mdContext->buf, in);
      mdi = 0;
    }
  }
}

/* The routine MD5Final terminates the message-digest computation and
   ends with the desired message digest in mdContext->digest[0...15].
 */
void MD5Final (MD5_CTX *mdContext)
{
  UINT4 in[16];
  int mdi;
  unsigned int i, ii;
  unsigned int padLen;

  /* save number of bits */
  in[14] = mdContext->i[0];
  in[15] = mdContext->i[1];

  /* compute number of bytes mod 64 */
  mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

  /* pad out to 56 mod 64 */
  padLen = (mdi < 56) ? (56 - mdi) : (120 - mdi);
  MD5Update (mdContext, PADDING, padLen);

  /* append length in bits and transform */
  for (i = 0, ii = 0; i < 14; i++, ii += 4)
    in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
            (((UINT4)mdContext->in[ii+2]) << 16) |
            (((UINT4)mdContext->in[ii+1]) << 8) |
            ((UINT4)mdContext->in[ii]);
  Transform (mdContext->buf, in);

  /* store buffer in digest */
  for (i = 0, ii = 0; i < 4; i++, ii += 4) {
    mdContext->digest[ii] = (unsigned char)(mdContext->buf[i] & 0xFF);
    mdContext->digest[ii+1] =
      (unsigned char)((mdContext->buf[i] >> 8) & 0xFF);
    mdContext->digest[ii+2] =
      (unsigned char)((mdContext->buf[i] >> 16) & 0xFF);
    mdContext->digest[ii+3] =
      (unsigned char)((mdContext->buf[i] >> 24) & 0xFF);
  }
}

/* Basic MD5 step. Transforms buf based on in.
 */
static void Transform (UINT4 *buf, UINT4 *in)
{
  UINT4 a = buf[0], b = buf[1], c = buf[2], d = buf[3];

  /* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
  FF ( a, b, c, d, in[ 0], S11, UL(3614090360ul)); /* 1 */
  FF ( d, a, b, c, in[ 1], S12, UL(3905402710ul)); /* 2 */
  FF ( c, d, a, b, in[ 2], S13, UL( 606105819ul)); /* 3 */
  FF ( b, c, d, a, in[ 3], S14, UL(3250441966ul)); /* 4 */
  FF ( a, b, c, d, in[ 4], S11, UL(4118548399ul)); /* 5 */
  FF ( d, a, b, c, in[ 5], S12, UL(1200080426ul)); /* 6 */
  FF ( c, d, a, b, in[ 6], S13, UL(2821735955ul)); /* 7 */
  FF ( b, c, d, a, in[ 7], S14, UL(4249261313ul)); /* 8 */
  FF ( a, b, c, d, in[ 8], S11, UL(1770035416ul)); /* 9 */
  FF ( d, a, b, c, in[ 9], S12, UL(2336552879ul)); /* 10 */
  FF ( c, d, a, b, in[10], S13, UL(4294925233ul)); /* 11 */
  FF ( b, c, d, a, in[11], S14, UL(2304563134ul)); /* 12 */
  FF ( a, b, c, d, in[12], S11, UL(1804603682ul)); /* 13 */
  FF ( d, a, b, c, in[13], S12, UL(4254626195ul)); /* 14 */
  FF ( c, d, a, b, in[14], S13, UL(2792965006ul)); /* 15 */
  FF ( b, c, d, a, in[15], S14, UL(1236535329ul)); /* 16 */

  /* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
  GG ( a, b, c, d, in[ 1], S21, UL(4129170786ul)); /* 17 */
  GG ( d, a, b, c, in[ 6], S22, UL(3225465664ul)); /* 18 */
  GG ( c, d, a, b, in[11], S23, UL( 643717713ul)); /* 19 */
  GG ( b, c, d, a, in[ 0], S24, UL(3921069994ul)); /* 20 */
  GG ( a, b, c, d, in[ 5], S21, UL(3593408605ul)); /* 21 */
  GG ( d, a, b, c, in[10], S22, UL(  38016083ul)); /* 22 */
  GG ( c, d, a, b, in[15], S23, UL(3634488961ul)); /* 23 */
  GG ( b, c, d, a, in[ 4], S24, UL(3889429448ul)); /* 24 */
  GG ( a, b, c, d, in[ 9], S21, UL( 568446438ul)); /* 25 */
  GG ( d, a, b, c, in[14], S22, UL(3275163606ul)); /* 26 */
  GG ( c, d, a, b, in[ 3], S23, UL(4107603335ul)); /* 27 */
  GG ( b, c, d, a, in[ 8], S24, UL(1163531501ul)); /* 28 */
  GG ( a, b, c, d, in[13], S21, UL(2850285829ul)); /* 29 */
  GG ( d, a, b, c, in[ 2], S22, UL(4243563512ul)); /* 30 */
  GG ( c, d, a, b, in[ 7], S23, UL(1735328473ul)); /* 31 */
  GG ( b, c, d, a, in[12], S24, UL(2368359562ul)); /* 32 */

  /* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23
  HH ( a, b, c, d, in[ 5], S31, UL(4294588738ul)); /* 33 */
  HH ( d, a, b, c, in[ 8], S32, UL(2272392833ul)); /* 34 */
  HH ( c, d, a, b, in[11], S33, UL(1839030562ul)); /* 35 */
  HH ( b, c, d, a, in[14], S34, UL(4259657740ul)); /* 36 */
  HH ( a, b, c, d, in[ 1], S31, UL(2763975236ul)); /* 37 */
  HH ( d, a, b, c, in[ 4], S32, UL(1272893353ul)); /* 38 */
  HH ( c, d, a, b, in[ 7], S33, UL(4139469664ul)); /* 39 */
  HH ( b, c, d, a, in[10], S34, UL(3200236656ul)); /* 40 */
  HH ( a, b, c, d, in[13], S31, UL( 681279174ul)); /* 41 */
  HH ( d, a, b, c, in[ 0], S32, UL(3936430074ul)); /* 42 */
  HH ( c, d, a, b, in[ 3], S33, UL(3572445317ul)); /* 43 */
  HH ( b, c, d, a, in[ 6], S34, UL(  76029189ul)); /* 44 */
  HH ( a, b, c, d, in[ 9], S31, UL(3654602809ul)); /* 45 */
  HH ( d, a, b, c, in[12], S32, UL(3873151461ul)); /* 46 */
  HH ( c, d, a, b, in[15], S33, UL( 530742520ul)); /* 47 */
  HH ( b, c, d, a, in[ 2], S34, UL(3299628645ul)); /* 48 */

  /* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
  II ( a, b, c, d, in[ 0], S41, UL(4096336452ul)); /* 49 */
  II ( d, a, b, c, in[ 7], S42, UL(1126891415ul)); /* 50 */
  II ( c, d, a, b, in[14], S43, UL(2878612391ul)); /* 51 */
  II ( b, c, d, a, in[ 5], S44, UL(4237533241ul)); /* 52 */
  II ( a, b, c, d, in[12], S41, UL(1700485571ul)); /* 53 */
  II ( d, a, b, c, in[ 3], S42, UL(2399980690ul)); /* 54 */
  II ( c, d, a, b, in[10], S43, UL(4293915773ul)); /* 55 */
  II ( b, c, d, a, in[ 1], S44, UL(2240044497ul)); /* 56 */
  II ( a, b, c, d, in[ 8], S41, UL(1873313359ul)); /* 57 */
  II ( d, a, b, c, in[15], S42, UL(4264355552ul)); /* 58 */
  II ( c, d, a, b, in[ 6], S43, UL(2734768916ul)); /* 59 */
  II ( b, c, d, a, in[13], S44, UL(1309151649ul)); /* 60 */
  II ( a, b, c, d, in[ 4], S41, UL(4149444226ul)); /* 61 */
  II ( d, a, b, c, in[11], S42, UL(3174756917ul)); /* 62 */
  II ( c, d, a, b, in[ 2], S43, UL( 718787259ul)); /* 63 */
  II ( b, c, d, a, in[ 9], S44, UL(3951481745ul)); /* 64 */

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;
}
