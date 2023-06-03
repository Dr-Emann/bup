/*
 * Copyright 2011 Avery Pennarun. All rights reserved.
 * 
 * (This license applies to bupsplit.c and bupsplit.h only.)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY AVERY PENNARUN AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __BUPSPLIT_H
#define __BUPSPLIT_H
#include <string.h>
#include <stdint.h>

#define BUP_WINDOWBITS (6)
#define BUP_WINDOWSIZE (1<<BUP_WINDOWBITS)

// According to librsync/rollsum.h:
// "We should make this something other than zero to improve the
// checksum algorithm: tridge suggests a prime number."
// apenwarr: I unscientifically tried 0 and 7919, and they both ended up
// slightly worse than the librsync value of 31 for my arbitrary test data.
#define ROLLSUM_CHAR_OFFSET 31

typedef struct {
    unsigned s1, s2;
} Rollsum;

static inline void rollsum_init(Rollsum *r)
{
    r->s1 = BUP_WINDOWSIZE * ROLLSUM_CHAR_OFFSET;
    r->s2 = BUP_WINDOWSIZE * (BUP_WINDOWSIZE-1) * ROLLSUM_CHAR_OFFSET;
}

// These formulas are based on rollsum.h in the librsync project.
static inline void rollsum_add(Rollsum *r, uint8_t drop, uint8_t add)
{
    r->s1 += add - drop;
    r->s2 += r->s1 - (BUP_WINDOWSIZE * (drop + ROLLSUM_CHAR_OFFSET));
}

static inline uint32_t rollsum_digest(Rollsum *r)
{
    return (r->s1 << 16) | (r->s2 & 0xffff);
}

#if defined(__has_builtin)
#if __has_builtin(__builtin_ctz)
#define BUPSLIT_HAS_BUILTIN_CTZ 1
#endif
#endif

static inline unsigned rollsum_extra_digest_bits(uint32_t digest, unsigned int nbits)
{
    digest >>= nbits;
    /*
     * See the DESIGN document, the bit counting loop used to
     * be written in a way that shifted rsum *before* checking
     * the lowest bit, make that explicit now so the code is a
     * bit easier to understand.
     */
    digest >>= 1;
#if BUPSLIT_HAS_BUILTIN_CTZ
    // SAFETY: `~digest` will never be 0, since we shift in at least 1 zero (which becomes a one)
    return __builtin_ctz(~digest);
#else
    unsigned extra_bits = 0;
    while (digest & 1) {
        extra_bits++;
        digest >>= 1;
    }
    return extra_bits;
#endif
}

uint32_t rollsum_sum(const uint8_t *buf, size_t ofs, size_t len);
int bupsplit_selftest(void);

#endif /* __BUPSPLIT_H */
