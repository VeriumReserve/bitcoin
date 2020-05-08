/*
 * Copyright 2009 Colin Percival, 2011 ArtForz, 2011-2014 pooler
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file was originally written by Colin Percival as part of the Tarsnap
 * online backup system.
 */

#include "scrypt.h"
#include "compat.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

static const uint32_t sha256_h[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

void sha256_init(uint32_t *state)
{
    memcpy(state, sha256_h, 32);
}

#if defined(__i386__) || defined(__aarch64__)

static const uint32_t sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* Elementary functions used by SHA256 */
#define Ch(x, y, z)     ((x & (y ^ z)) ^ z)
#define Maj(x, y, z)    ((x & (y | z)) | (y & z))
#define ROTR(x, n)      ((x >> n) | (x << (32 - n)))
#define S0(x)           (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S1(x)           (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define s0(x)           (ROTR(x, 7) ^ ROTR(x, 18) ^ (x >> 3))
#define s1(x)           (ROTR(x, 17) ^ ROTR(x, 19) ^ (x >> 10))

/* SHA256 round function */
#define RND(a, b, c, d, e, f, g, h, k) \
    do { \
        t0 = h + S1(e) + Ch(e, f, g) + k; \
        t1 = S0(a) + Maj(a, b, c); \
        d += t0; \
        h  = t0 + t1; \
    } while (0)

/* Adjusted round function for rotating state */
#define RNDr(S, W, i) \
    RND(S[(64 - i) % 8], S[(65 - i) % 8], \
        S[(66 - i) % 8], S[(67 - i) % 8], \
        S[(68 - i) % 8], S[(69 - i) % 8], \
        S[(70 - i) % 8], S[(71 - i) % 8], \
        W[i] + sha256_k[i])

/*
 * SHA256 block compression function.  The 256-bit state is transformed via
 * the 512-bit input block to produce a new state.
 */
void sha256_transform(uint32_t *state, const uint32_t *block, int swap)
{
    uint32_t W[64];
    uint32_t S[8];
    uint32_t t0, t1;
    int i;

    /* 1. Prepare message schedule W. */
    if (swap) {
        for (i = 0; i < 16; i++)
            W[i] = swab32(block[i]);
    } else
        memcpy(W, block, 64);
    for (i = 16; i < 64; i += 2) {
        W[i]   = s1(W[i - 2]) + W[i - 7] + s0(W[i - 15]) + W[i - 16];
        W[i+1] = s1(W[i - 1]) + W[i - 6] + s0(W[i - 14]) + W[i - 15];
    }

    /* 2. Initialize working variables. */
    memcpy(S, state, 32);

    /* 3. Mix. */
    RNDr(S, W,  0);
    RNDr(S, W,  1);
    RNDr(S, W,  2);
    RNDr(S, W,  3);
    RNDr(S, W,  4);
    RNDr(S, W,  5);
    RNDr(S, W,  6);
    RNDr(S, W,  7);
    RNDr(S, W,  8);
    RNDr(S, W,  9);
    RNDr(S, W, 10);
    RNDr(S, W, 11);
    RNDr(S, W, 12);
    RNDr(S, W, 13);
    RNDr(S, W, 14);
    RNDr(S, W, 15);
    RNDr(S, W, 16);
    RNDr(S, W, 17);
    RNDr(S, W, 18);
    RNDr(S, W, 19);
    RNDr(S, W, 20);
    RNDr(S, W, 21);
    RNDr(S, W, 22);
    RNDr(S, W, 23);
    RNDr(S, W, 24);
    RNDr(S, W, 25);
    RNDr(S, W, 26);
    RNDr(S, W, 27);
    RNDr(S, W, 28);
    RNDr(S, W, 29);
    RNDr(S, W, 30);
    RNDr(S, W, 31);
    RNDr(S, W, 32);
    RNDr(S, W, 33);
    RNDr(S, W, 34);
    RNDr(S, W, 35);
    RNDr(S, W, 36);
    RNDr(S, W, 37);
    RNDr(S, W, 38);
    RNDr(S, W, 39);
    RNDr(S, W, 40);
    RNDr(S, W, 41);
    RNDr(S, W, 42);
    RNDr(S, W, 43);
    RNDr(S, W, 44);
    RNDr(S, W, 45);
    RNDr(S, W, 46);
    RNDr(S, W, 47);
    RNDr(S, W, 48);
    RNDr(S, W, 49);
    RNDr(S, W, 50);
    RNDr(S, W, 51);
    RNDr(S, W, 52);
    RNDr(S, W, 53);
    RNDr(S, W, 54);
    RNDr(S, W, 55);
    RNDr(S, W, 56);
    RNDr(S, W, 57);
    RNDr(S, W, 58);
    RNDr(S, W, 59);
    RNDr(S, W, 60);
    RNDr(S, W, 61);
    RNDr(S, W, 62);
    RNDr(S, W, 63);

    /* 4. Mix local working variables into global state */
    for (i = 0; i < 8; i++)
        state[i] += S[i];
}
#endif

static const uint32_t keypad[12] = {
	0x80000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000280
};
static const uint32_t innerpad[11] = {
	0x80000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x000004a0
};
static const uint32_t outerpad[8] = {
	0x80000000, 0, 0, 0, 0, 0, 0, 0x00000300
};
static const uint32_t finalblk[16] = {
	0x00000001, 0x80000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000620
};


#ifndef __FreeBSD__
static inline void be32enc(void *pp, uint32_t x)
{
    uint8_t *p = (uint8_t *)pp;
    p[3] = x & 0xff;
    p[2] = (x >> 8) & 0xff;
    p[1] = (x >> 16) & 0xff;
    p[0] = (x >> 24) & 0xff;
}

static inline uint32_t be32dec(const void *pp)
{
    const uint8_t *p = (uint8_t const *)pp;
    return ((uint32_t)(p[3]) + ((uint32_t)(p[2]) << 8) +
        ((uint32_t)(p[1]) << 16) + ((uint32_t)(p[0]) << 24));
}
#endif

static inline void HMAC_SHA256_80_init(const uint32_t *key,
	uint32_t *tstate, uint32_t *ostate)
{
	uint32_t ihash[8];
	uint32_t pad[16];
	int i;

	/* tstate is assumed to contain the midstate of key */
	memcpy(pad, key + 16, 16);
	memcpy(pad + 4, keypad, 48);
	sha256_transform(tstate, pad, 0);
	memcpy(ihash, tstate, 32);

	sha256_init(ostate);
	for (i = 0; i < 8; i++)
		pad[i] = ihash[i] ^ 0x5c5c5c5c;
	for (; i < 16; i++)
		pad[i] = 0x5c5c5c5c;
	sha256_transform(ostate, pad, 0);

	sha256_init(tstate);
	for (i = 0; i < 8; i++)
		pad[i] = ihash[i] ^ 0x36363636;
	for (; i < 16; i++)
		pad[i] = 0x36363636;
	sha256_transform(tstate, pad, 0);
}

static inline void PBKDF2_SHA256_80_128(const uint32_t *tstate,
	const uint32_t *ostate, const uint32_t *salt, uint32_t *output)
{
	uint32_t istate[8], ostate2[8];
	uint32_t ibuf[16], obuf[16];
	int i, j;

	memcpy(istate, tstate, 32);
	sha256_transform(istate, salt, 0);
	
	memcpy(ibuf, salt + 16, 16);
	memcpy(ibuf + 5, innerpad, 44);
	memcpy(obuf + 8, outerpad, 32);

	for (i = 0; i < 4; i++) {
		memcpy(obuf, istate, 32);
		ibuf[4] = i + 1;
		sha256_transform(obuf, ibuf, 0);

		memcpy(ostate2, ostate, 32);
		sha256_transform(ostate2, obuf, 0);
		for (j = 0; j < 8; j++)
			output[8 * i + j] = swab32(ostate2[j]);
	}
}

static inline void PBKDF2_SHA256_128_32(uint32_t *tstate, uint32_t *ostate,
    const uint32_t *salt, uint32_t *output)
{
	uint32_t buf[16];
	int i;
	
	sha256_transform(tstate, salt, 1);
	sha256_transform(tstate, salt + 16, 1);
	sha256_transform(tstate, finalblk, 0);
	memcpy(buf, tstate, 32);
	memcpy(buf + 8, outerpad, 32);

	sha256_transform(ostate, buf, 0);
	for (i = 0; i < 8; i++)
		output[i] = swab32(ostate[i]);
}


#ifdef HAVE_SHA256_4WAY

static const uint32_t keypad_4way[4 * 12] = {
	0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000280, 0x00000280, 0x00000280, 0x00000280
};
static const uint32_t innerpad_4way[4 * 11] = {
	0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x000004a0, 0x000004a0, 0x000004a0, 0x000004a0
};
static const uint32_t outerpad_4way[4 * 8] = {
	0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000300, 0x00000300, 0x00000300, 0x00000300
};
static const uint32_t finalblk_4way[4 * 16] __attribute__((aligned(16))) = {
	0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000620, 0x00000620, 0x00000620, 0x00000620
};

static inline void HMAC_SHA256_80_init_4way(const uint32_t *key,
	uint32_t *tstate, uint32_t *ostate)
{
	uint32_t ihash[4 * 8] __attribute__((aligned(16)));
	uint32_t pad[4 * 16] __attribute__((aligned(16)));
	int i;

	/* tstate is assumed to contain the midstate of key */
	memcpy(pad, key + 4 * 16, 4 * 16);
	memcpy(pad + 4 * 4, keypad_4way, 4 * 48);
	sha256_transform_4way(tstate, pad, 0);
	memcpy(ihash, tstate, 4 * 32);

	sha256_init_4way(ostate);
	for (i = 0; i < 4 * 8; i++)
		pad[i] = ihash[i] ^ 0x5c5c5c5c;
	for (; i < 4 * 16; i++)
		pad[i] = 0x5c5c5c5c;
	sha256_transform_4way(ostate, pad, 0);

	sha256_init_4way(tstate);
	for (i = 0; i < 4 * 8; i++)
		pad[i] = ihash[i] ^ 0x36363636;
	for (; i < 4 * 16; i++)
		pad[i] = 0x36363636;
	sha256_transform_4way(tstate, pad, 0);
}

static inline void PBKDF2_SHA256_80_128_4way(const uint32_t *tstate,
	const uint32_t *ostate, const uint32_t *salt, uint32_t *output)
{
	uint32_t istate[4 * 8] __attribute__((aligned(16)));
	uint32_t ostate2[4 * 8] __attribute__((aligned(16)));
	uint32_t ibuf[4 * 16] __attribute__((aligned(16)));
	uint32_t obuf[4 * 16] __attribute__((aligned(16)));
	int i, j;

	memcpy(istate, tstate, 4 * 32);
	sha256_transform_4way(istate, salt, 0);
	
	memcpy(ibuf, salt + 4 * 16, 4 * 16);
	memcpy(ibuf + 4 * 5, innerpad_4way, 4 * 44);
	memcpy(obuf + 4 * 8, outerpad_4way, 4 * 32);

	for (i = 0; i < 4; i++) {
		memcpy(obuf, istate, 4 * 32);
		ibuf[4 * 4 + 0] = i + 1;
		ibuf[4 * 4 + 1] = i + 1;
		ibuf[4 * 4 + 2] = i + 1;
		ibuf[4 * 4 + 3] = i + 1;
		sha256_transform_4way(obuf, ibuf, 0);

		memcpy(ostate2, ostate, 4 * 32);
		sha256_transform_4way(ostate2, obuf, 0);
		for (j = 0; j < 4 * 8; j++)
			output[4 * 8 * i + j] = swab32(ostate2[j]);
	}
}

static inline void PBKDF2_SHA256_128_32_4way(uint32_t *tstate,
	uint32_t *ostate, const uint32_t *salt, uint32_t *output)
{
	uint32_t buf[4 * 16] __attribute__((aligned(16)));
	int i;
	
	sha256_transform_4way(tstate, salt, 1);
	sha256_transform_4way(tstate, salt + 4 * 16, 1);
	sha256_transform_4way(tstate, finalblk_4way, 0);
	memcpy(buf, tstate, 4 * 32);
	memcpy(buf + 4 * 8, outerpad_4way, 4 * 32);

	sha256_transform_4way(ostate, buf, 0);
	for (i = 0; i < 4 * 8; i++)
		output[i] = swab32(ostate[i]);
}

#endif /* HAVE_SHA256_4WAY */


#ifdef HAVE_SHA256_8WAY

static const uint32_t finalblk_8way[8 * 16] __attribute__((aligned(32))) = {
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000620, 0x00000620, 0x00000620, 0x00000620, 0x00000620, 0x00000620, 0x00000620, 0x00000620
};

static inline void HMAC_SHA256_80_init_8way(const uint32_t *key,
	uint32_t *tstate, uint32_t *ostate)
{
	uint32_t ihash[8 * 8] __attribute__((aligned(32)));
	uint32_t pad[8 * 16] __attribute__((aligned(32)));
	int i;
	
	/* tstate is assumed to contain the midstate of key */
	memcpy(pad, key + 8 * 16, 8 * 16);
	for (i = 0; i < 8; i++)
		pad[8 * 4 + i] = 0x80000000;
	memset(pad + 8 * 5, 0x00, 8 * 40);
	for (i = 0; i < 8; i++)
		pad[8 * 15 + i] = 0x00000280;
	sha256_transform_8way(tstate, pad, 0);
	memcpy(ihash, tstate, 8 * 32);
	
	sha256_init_8way(ostate);
	for (i = 0; i < 8 * 8; i++)
		pad[i] = ihash[i] ^ 0x5c5c5c5c;
	for (; i < 8 * 16; i++)
		pad[i] = 0x5c5c5c5c;
	sha256_transform_8way(ostate, pad, 0);
	
	sha256_init_8way(tstate);
	for (i = 0; i < 8 * 8; i++)
		pad[i] = ihash[i] ^ 0x36363636;
	for (; i < 8 * 16; i++)
		pad[i] = 0x36363636;
	sha256_transform_8way(tstate, pad, 0);
}

static inline void PBKDF2_SHA256_80_128_8way(const uint32_t *tstate,
	const uint32_t *ostate, const uint32_t *salt, uint32_t *output)
{
	uint32_t istate[8 * 8] __attribute__((aligned(32)));
	uint32_t ostate2[8 * 8] __attribute__((aligned(32)));
	uint32_t ibuf[8 * 16] __attribute__((aligned(32)));
	uint32_t obuf[8 * 16] __attribute__((aligned(32)));
	int i, j;
	
	memcpy(istate, tstate, 8 * 32);
	sha256_transform_8way(istate, salt, 0);
	
	memcpy(ibuf, salt + 8 * 16, 8 * 16);
	for (i = 0; i < 8; i++)
		ibuf[8 * 5 + i] = 0x80000000;
	memset(ibuf + 8 * 6, 0x00, 8 * 36);
	for (i = 0; i < 8; i++)
		ibuf[8 * 15 + i] = 0x000004a0;
	
	for (i = 0; i < 8; i++)
		obuf[8 * 8 + i] = 0x80000000;
	memset(obuf + 8 * 9, 0x00, 8 * 24);
	for (i = 0; i < 8; i++)
		obuf[8 * 15 + i] = 0x00000300;
	
	for (i = 0; i < 4; i++) {
		memcpy(obuf, istate, 8 * 32);
		ibuf[8 * 4 + 0] = i + 1;
		ibuf[8 * 4 + 1] = i + 1;
		ibuf[8 * 4 + 2] = i + 1;
		ibuf[8 * 4 + 3] = i + 1;
		ibuf[8 * 4 + 4] = i + 1;
		ibuf[8 * 4 + 5] = i + 1;
		ibuf[8 * 4 + 6] = i + 1;
		ibuf[8 * 4 + 7] = i + 1;
		sha256_transform_8way(obuf, ibuf, 0);
		
		memcpy(ostate2, ostate, 8 * 32);
		sha256_transform_8way(ostate2, obuf, 0);
		for (j = 0; j < 8 * 8; j++)
			output[8 * 8 * i + j] = swab32(ostate2[j]);
	}
}

static inline void PBKDF2_SHA256_128_32_8way(uint32_t *tstate,
	uint32_t *ostate, const uint32_t *salt, uint32_t *output)
{
	uint32_t buf[8 * 16] __attribute__((aligned(32)));
	int i;
	
	sha256_transform_8way(tstate, salt, 1);
	sha256_transform_8way(tstate, salt + 8 * 16, 1);
	sha256_transform_8way(tstate, finalblk_8way, 0);
	
	memcpy(buf, tstate, 8 * 32);
	for (i = 0; i < 8; i++)
		buf[8 * 8 + i] = 0x80000000;
	memset(buf + 8 * 9, 0x00, 8 * 24);
	for (i = 0; i < 8; i++)
		buf[8 * 15 + i] = 0x00000300;
	sha256_transform_8way(ostate, buf, 0);
	
	for (i = 0; i < 8 * 8; i++)
		output[i] = swab32(ostate[i]);
}

#endif /* HAVE_SHA256_8WAY */

#ifndef SCRYPT_MAX_WAYS
#define SCRYPT_MAX_WAYS 1
#define scrypt_best_throughput() 1
#endif

unsigned char *scrypt_buffer_alloc()
{
    return (unsigned char*)malloc((size_t)N * SCRYPT_MAX_WAYS * 128 + 63);
}

static void scrypt_N_1_1_256(const uint32_t *input, uint32_t *output, uint32_t *midstate, unsigned char *scratchpad)
{
	uint32_t tstate[8], ostate[8];
	uint32_t X[32] __attribute__((aligned(128)));
	uint32_t *V;
	
	V = (uint32_t *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));

	memcpy(tstate, midstate, 32);
	HMAC_SHA256_80_init(input, tstate, ostate);
	PBKDF2_SHA256_80_128(tstate, ostate, input, X);

	scrypt_core(X, V, N);

	PBKDF2_SHA256_128_32(tstate, ostate, X, output);
}

#ifdef HAVE_SHA256_4WAY
static void scrypt_N_1_1_256_4way(const uint32_t *input,
	uint32_t *output, uint32_t *midstate, unsigned char *scratchpad, int N)
{
	uint32_t tstate[4 * 8] __attribute__((aligned(128)));
	uint32_t ostate[4 * 8] __attribute__((aligned(128)));
	uint32_t W[4 * 32] __attribute__((aligned(128)));
	uint32_t X[4 * 32] __attribute__((aligned(128)));
	uint32_t *V;
	int i, k;
	
	V = (uint32_t *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));

	for (i = 0; i < 20; i++)
		for (k = 0; k < 4; k++)
			W[4 * i + k] = input[k * 20 + i];
	for (i = 0; i < 8; i++)
		for (k = 0; k < 4; k++)
			tstate[4 * i + k] = midstate[i];
	HMAC_SHA256_80_init_4way(W, tstate, ostate);
	PBKDF2_SHA256_80_128_4way(tstate, ostate, W, W);
	for (i = 0; i < 32; i++)
		for (k = 0; k < 4; k++)
			X[k * 32 + i] = W[4 * i + k];
	scrypt_core(X + 0 * 32, V, N);
	scrypt_core(X + 1 * 32, V, N);
	scrypt_core(X + 2 * 32, V, N);
	scrypt_core(X + 3 * 32, V, N);
	for (i = 0; i < 32; i++)
		for (k = 0; k < 4; k++)
			W[4 * i + k] = X[k * 32 + i];
	PBKDF2_SHA256_128_32_4way(tstate, ostate, W, W);
	for (i = 0; i < 8; i++)
		for (k = 0; k < 4; k++)
			output[k * 8 + i] = W[4 * i + k];
}
#endif /* HAVE_SHA256_4WAY */

#ifdef HAVE_SCRYPT_3WAY

static void scrypt_N_1_1_256_3way(const uint32_t *input,
	uint32_t *output, uint32_t *midstate, unsigned char *scratchpad, int N)
{
	uint32_t tstate[3 * 8], ostate[3 * 8];
	uint32_t X[3 * 32] __attribute__((aligned(64)));
	uint32_t *V;
	
	V = (uint32_t *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));

	memcpy(tstate +  0, midstate, 32);
	memcpy(tstate +  8, midstate, 32);
	memcpy(tstate + 16, midstate, 32);
	HMAC_SHA256_80_init(input +  0, tstate +  0, ostate +  0);
	HMAC_SHA256_80_init(input + 20, tstate +  8, ostate +  8);
	HMAC_SHA256_80_init(input + 40, tstate + 16, ostate + 16);
	PBKDF2_SHA256_80_128(tstate +  0, ostate +  0, input +  0, X +  0);
	PBKDF2_SHA256_80_128(tstate +  8, ostate +  8, input + 20, X + 32);
	PBKDF2_SHA256_80_128(tstate + 16, ostate + 16, input + 40, X + 64);

	scrypt_core_3way(X, V, N);

	PBKDF2_SHA256_128_32(tstate +  0, ostate +  0, X +  0, output +  0);
	PBKDF2_SHA256_128_32(tstate +  8, ostate +  8, X + 32, output +  8);
	PBKDF2_SHA256_128_32(tstate + 16, ostate + 16, X + 64, output + 16);
}

#ifdef HAVE_SHA256_4WAY
static void scrypt_N_1_1_256_12way(const uint32_t *input,
	uint32_t *output, uint32_t *midstate, unsigned char *scratchpad, int N)
{
	uint32_t tstate[12 * 8] __attribute__((aligned(128)));
	uint32_t ostate[12 * 8] __attribute__((aligned(128)));
	uint32_t W[12 * 32] __attribute__((aligned(128)));
    uint32_t X[12 * 32] __attribute__((aligned(128)));
	uint32_t *V;
	int i, j, k;
	
	V = (uint32_t *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));

	for (j = 0; j < 3; j++)
		for (i = 0; i < 20; i++)
			for (k = 0; k < 4; k++)
				W[128 * j + 4 * i + k] = input[80 * j + k * 20 + i];
	for (j = 0; j < 3; j++)
		for (i = 0; i < 8; i++)
			for (k = 0; k < 4; k++)
				tstate[32 * j + 4 * i + k] = midstate[i];
	HMAC_SHA256_80_init_4way(W +   0, tstate +  0, ostate +  0);
	HMAC_SHA256_80_init_4way(W + 128, tstate + 32, ostate + 32);
	HMAC_SHA256_80_init_4way(W + 256, tstate + 64, ostate + 64);
	PBKDF2_SHA256_80_128_4way(tstate +  0, ostate +  0, W +   0, W +   0);
	PBKDF2_SHA256_80_128_4way(tstate + 32, ostate + 32, W + 128, W + 128);
	PBKDF2_SHA256_80_128_4way(tstate + 64, ostate + 64, W + 256, W + 256);
	for (j = 0; j < 3; j++)
		for (i = 0; i < 32; i++)
			for (k = 0; k < 4; k++)
				X[128 * j + k * 32 + i] = W[128 * j + 4 * i + k];
	scrypt_core_3way(X + 0 * 96, V, N);
	scrypt_core_3way(X + 1 * 96, V, N);
	scrypt_core_3way(X + 2 * 96, V, N);
	scrypt_core_3way(X + 3 * 96, V, N);
	for (j = 0; j < 3; j++)
		for (i = 0; i < 32; i++)
			for (k = 0; k < 4; k++)
				W[128 * j + 4 * i + k] = X[128 * j + k * 32 + i];
	PBKDF2_SHA256_128_32_4way(tstate +  0, ostate +  0, W +   0, W +   0);
	PBKDF2_SHA256_128_32_4way(tstate + 32, ostate + 32, W + 128, W + 128);
	PBKDF2_SHA256_128_32_4way(tstate + 64, ostate + 64, W + 256, W + 256);
	for (j = 0; j < 3; j++)
		for (i = 0; i < 8; i++)
			for (k = 0; k < 4; k++)
				output[32 * j + k * 8 + i] = W[128 * j + 4 * i + k];
}
#endif /* HAVE_SHA256_4WAY */

#endif /* HAVE_SCRYPT_3WAY */

#ifdef HAVE_SCRYPT_6WAY
static void scrypt_N_1_1_256_24way(const uint32_t *input,
	uint32_t *output, uint32_t *midstate, unsigned char *scratchpad, int N)
{
	uint32_t tstate[24 * 8] __attribute__((aligned(128)));
	uint32_t ostate[24 * 8] __attribute__((aligned(128)));
	uint32_t W[24 * 32] __attribute__((aligned(128)));
	uint32_t X[24 * 32] __attribute__((aligned(128)));
	uint32_t *V;
	int i, j, k;
	
	V = (uint32_t *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));
	
	for (j = 0; j < 3; j++) 
		for (i = 0; i < 20; i++)
			for (k = 0; k < 8; k++)
				W[8 * 32 * j + 8 * i + k] = input[8 * 20 * j + k * 20 + i];
	for (j = 0; j < 3; j++)
		for (i = 0; i < 8; i++)
			for (k = 0; k < 8; k++)
				tstate[8 * 8 * j + 8 * i + k] = midstate[i];
	HMAC_SHA256_80_init_8way(W +   0, tstate +   0, ostate +   0);
	HMAC_SHA256_80_init_8way(W + 256, tstate +  64, ostate +  64);
	HMAC_SHA256_80_init_8way(W + 512, tstate + 128, ostate + 128);
	PBKDF2_SHA256_80_128_8way(tstate +   0, ostate +   0, W +   0, W +   0);
	PBKDF2_SHA256_80_128_8way(tstate +  64, ostate +  64, W + 256, W + 256);
	PBKDF2_SHA256_80_128_8way(tstate + 128, ostate + 128, W + 512, W + 512);
	for (j = 0; j < 3; j++)
		for (i = 0; i < 32; i++)
			for (k = 0; k < 8; k++)
				X[8 * 32 * j + k * 32 + i] = W[8 * 32 * j + 8 * i + k];
	scrypt_core_6way(X + 0 * 32, V, N);
	scrypt_core_6way(X + 6 * 32, V, N);
	scrypt_core_6way(X + 12 * 32, V, N);
	scrypt_core_6way(X + 18 * 32, V, N);
	for (j = 0; j < 3; j++)
		for (i = 0; i < 32; i++)
			for (k = 0; k < 8; k++)
				W[8 * 32 * j + 8 * i + k] = X[8 * 32 * j + k * 32 + i];
	PBKDF2_SHA256_128_32_8way(tstate +   0, ostate +   0, W +   0, W +   0);
	PBKDF2_SHA256_128_32_8way(tstate +  64, ostate +  64, W + 256, W + 256);
	PBKDF2_SHA256_128_32_8way(tstate + 128, ostate + 128, W + 512, W + 512);
	for (j = 0; j < 3; j++)
		for (i = 0; i < 8; i++)
			for (k = 0; k < 8; k++)
				output[8 * 8 * j + k * 8 + i] = W[8 * 32 * j + 8 * i + k];
}
#endif /* HAVE_SCRYPT_6WAY */

bool fulltest(const uint32_t *hash, const uint32_t *target)
{
	int i;
	for (i = 7; i >= 0; i--) {
		if (hash[i] > target[i]) {
			return false;
		}
		if (hash[i] < target[i]) {
			return true;
		}
	}

	return true;
}

bool scrypt_N_1_1_256_multi(void *input, uint256 hashTarget, int *nHashesDone, unsigned char *scratchbuf)
{
	uint32_t pdata[20];
	uint32_t data[SCRYPT_MAX_WAYS * 20];
	uint32_t dhash[SCRYPT_MAX_WAYS * 8];
	uint32_t midstate[8];
	uint32_t n;
	int throughput = scrypt_best_throughput();
	int i;

	for (int i = 0; i < 20; i++)
		pdata[i] = be32dec(&((const uint32_t *)input)[i]);
	n = pdata[19];
	
#ifdef HAVE_SHA256_4WAY
	if (sha256_use_4way())
		throughput *= 4;
#endif
	
	for (i = 0; i < throughput; i++)
		memcpy(data + i * 20, pdata, 80);
	
	sha256_init(midstate);
	sha256_transform(midstate, data, 0);
	
	for (i = 1; i < throughput; i++)
		data[i * 20 + 19] = ++n;
		
#if defined(HAVE_SHA256_4WAY)
	if (throughput == 4)
        scrypt_N_1_1_256_4way(data, dhash, midstate, scratchbuf, N);
	else
#endif
#if defined(HAVE_SCRYPT_3WAY) && defined(HAVE_SHA256_4WAY)
	if (throughput == 12)
        scrypt_N_1_1_256_12way(data, dhash, midstate, scratchbuf, N);
	else
#endif
#if defined(HAVE_SCRYPT_6WAY)
	if (throughput == 24)
        scrypt_N_1_1_256_24way(data, dhash, midstate, scratchbuf, N);
	else
#endif
#if defined(HAVE_SCRYPT_3WAY)
	if (throughput == 3)
        scrypt_N_1_1_256_3way(data, dhash, midstate, scratchbuf, N);
	else
#endif
		scrypt_N_1_1_256(data, dhash, midstate, scratchbuf);
		
	*nHashesDone = throughput;

	for (i = 0; i < throughput; i++) {
		if (fulltest(dhash + i * 8, (uint32_t*)(BEGIN(hashTarget)))) {
			be32enc(&((uint32_t *)input)[19], data[i * 20 + 19]);
			return true;
		}
	}
	return false;
}

void scryptHash(const void *input, char *output)
{
    uint32_t midstate[8];
    uint32_t data[20];
    unsigned char *scratchbuf = scrypt_buffer_alloc();

    memset(output, 0, 32);
    if (!scratchbuf)
        return;

    for (int i = 0; i < 20; i++)
        data[i] = be32dec(&((const uint32_t *)input)[i]);

    sha256_init(midstate);
    sha256_transform(midstate, data, 0);

    scrypt_N_1_1_256(data, (uint32_t*)output, midstate, scratchbuf);

    free(scratchbuf);
}
