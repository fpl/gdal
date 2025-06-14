/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */

/* This code was modified in 1997 by Jim Kingdon of Cyclic Software to
not require an integer type which is exactly 32 bits.  This work
draws on the changes for the same purpose by Tatu Ylonen
<ylo@cs.hut.fi> as part of SSH, but since I didn't actually use
that code, there is no copyright issue.  I hereby disclaim
copyright in any changes I have made; this code remains in the
public domain.  */

/* Note regarding cvs_* namespace: this avoids potential conflicts
with libraries such as some versions of Kerberos.  No particular
need to worry about whether the system supplies an MD5 library, as
this file is only about 3k of object code.  */

/* Modified by E. Rouault, to fix :
   warning: argument to 'sizeof' in 'memset' call is the same expression as
   the destination; did you mean to dereference it? [-Wsizeof-pointer-memaccess]
        memset(ctx, 0, sizeof(ctx)); */   /* In case it is sensitive */
/* at the end of cvs_MD5Final */

#include "cpl_md5.h"

#include "cpl_string.h"

static GUInt32 getu32(const unsigned char *addr)
{
    return ((((static_cast<GUInt32>(addr[3]) << 8) | addr[2]) << 8) | addr[1])
               << 8 |
           addr[0];
}

static void putu32(GUInt32 data, unsigned char *addr)
{
    addr[0] = static_cast<unsigned char>(data & 0xff);
    addr[1] = static_cast<unsigned char>((data >> 8) & 0xff);
    addr[2] = static_cast<unsigned char>((data >> 16) & 0xff);
    addr[3] = static_cast<unsigned char>((data >> 24) & 0xff);
}

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void CPLMD5Init(struct CPLMD5Context *context)
{
    context->buf[0] = 0x67452301;
    context->buf[1] = 0xefcdab89;
    context->buf[2] = 0x98badcfe;
    context->buf[3] = 0x10325476;

    context->bits[0] = 0;
    context->bits[1] = 0;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void CPLMD5Update(struct CPLMD5Context *context, const void *buf, size_t len)
{
    const GByte *pabyBuf = static_cast<const GByte *>(buf);
    while (len > 0xffffffffU)
    {
        CPLMD5Update(context, pabyBuf, 0xffffffffU);
        pabyBuf += 0xffffffffU;
        len -= 0xffffffffU;
    }

    // Update bitcount
    GUInt32 t = context->bits[0];
    // Clear top 3 bits before left shifting to avoid Coverity Scan warning
    // about 0xffffffffU overflowing during the shit.
    const GUInt32 lenShifted = static_cast<GUInt32>(len & 0x1fffffffU) << 3U;
    context->bits[0] = (t + lenShifted) & 0xffffffff;
    if (context->bits[0] < t)
        context->bits[1]++; /* Carry from low to high */
    context->bits[1] += static_cast<GUInt32>(len >> 29);

    t = (t >> 3) & 0x3f; /* Bytes already in shsInfo->data */

    /* Handle any leading odd-sized chunks */

    if (t)
    {
        unsigned char *p = context->in + t;

        t = 64 - t;
        if (len < t)
        {
            memcpy(p, pabyBuf, len);
            return;
        }
        memcpy(p, pabyBuf, t);
        CPLMD5Transform(context->buf, context->in);
        pabyBuf += t;
        len -= t;
    }

    /* Process data in 64-byte chunks */

    while (len >= 64)
    {
        memcpy(context->in, pabyBuf, 64);
        CPLMD5Transform(context->buf, context->in);
        pabyBuf += 64;
        len -= 64;
    }

    /* Handle any remaining bytes of data. */

    memcpy(context->in, pabyBuf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void CPLMD5Final(unsigned char digest[16], struct CPLMD5Context *context)
{
    /* Compute number of bytes mod 64 */
    unsigned count = static_cast<unsigned>((context->bits[0] >> 3) & 0x3F);

    /* Set the first char of padding to 0x80.  This is safe since there is
    always at least one byte free */
    unsigned char *p = context->in + count;
    *p++ = 0x80;

    /* Bytes of padding needed to make 64 bytes */
    count = 64 - 1 - count;

    /* Pad out to 56 mod 64 */
    if (count < 8)
    {
        /* Two lots of padding:  Pad the first block to 64 bytes */
        memset(p, 0, count);
        CPLMD5Transform(context->buf, context->in);

        /* Now fill the next block with 56 bytes */
        memset(context->in, 0, 56);
    }
    else
    {
        /* Pad block to 56 bytes */
        memset(p, 0, count - 8);
    }

    /* Append length in bits and transform */
    putu32(context->bits[0], context->in + 56);
    putu32(context->bits[1], context->in + 60);

    CPLMD5Transform(context->buf, context->in);
    putu32(context->buf[0], digest);
    putu32(context->buf[1], digest + 4);
    putu32(context->buf[2], digest + 8);
    putu32(context->buf[3], digest + 12);
    memset(context, 0, sizeof(*context)); /* In case it is sensitive */
}

#ifndef ASM_MD5

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s)                                        \
    (w += f(x, y, z) + data, w &= 0xffffffff, w = w << s | w >> (32 - s),      \
     w += x)

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
CPL_NOSANITIZE_UNSIGNED_INT_OVERFLOW
void CPLMD5Transform(GUInt32 buf[4], const unsigned char inraw[64])
{
    GUInt32 in[16];
    for (int i = 0; i < 16; ++i)
        in[i] = getu32(inraw + 4 * i);

    GUInt32 a = buf[0];
    GUInt32 b = buf[1];
    GUInt32 c = buf[2];
    GUInt32 d = buf[3];

    MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
    MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
    MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
    MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
    MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
    MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
    MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
    MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
    MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
    MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
    MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
    MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
    MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
    MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
    MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
    MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
    MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
    MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
    MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
    MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
    MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}
#endif

/**
 * @brief CPLMD5String Transform string to MD5 hash
 * @param pszText Text to transform
 * @return MD5 hash string
 */
const char *CPLMD5String(const char *pszText)
{
    struct CPLMD5Context context;
    CPLMD5Init(&context);
    CPLMD5Update(&context, pszText, strlen(pszText));
    unsigned char hash[16];
    CPLMD5Final(hash, &context);

    constexpr char tohex[] = "0123456789abcdef";
    char hhash[33];
    for (int i = 0; i < 16; ++i)
    {
        hhash[i * 2] = tohex[(hash[i] >> 4) & 0xf];
        hhash[i * 2 + 1] = tohex[hash[i] & 0xf];
    }
    hhash[32] = '\0';
    return CPLSPrintf("%s", hhash);
}
