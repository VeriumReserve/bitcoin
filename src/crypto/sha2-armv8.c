/*
 *  ARMv8-A Cryptography Extension SHA256 support functions
 *
 *  Copyright (C) 2016, CriticalBlue Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */


#include <stdint.h>
#include <string.h>
#include <arm_neon.h>

static const uint32_t sha256_h[8] = {
	0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
	0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

static const uint32_t K[] = {
	0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
	0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
	0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
	0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
	0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
	0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
	0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
	0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
	0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
	0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
	0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
	0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
	0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
	0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
	0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
	0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};

void sha256_init(uint32_t *state)
{
	memcpy(state, sha256_h, 32);
}

#define Rx(T0, T1, K, W0, W1, W2, W3)      \
	W0 = vsha256su0q_u32( W0, W1 );    \
	d2 = d0;                           \
	T1 = vaddq_u32( W1, K );           \
	d0 = vsha256hq_u32( d0, d1, T0 );  \
	d1 = vsha256h2q_u32( d1, d2, T0 ); \
	W0 = vsha256su1q_u32( W0, W2, W3 );

#define Ry(T0, T1, K, W1)                  \
	d2 = d0;                           \
	T1 = vaddq_u32( W1, K  );          \
	d0 = vsha256hq_u32( d0, d1, T0 );  \
	d1 = vsha256h2q_u32( d1, d2, T0 );

#define Rz(T0)                             \
	d2 = d0;                       	   \
	d0 = vsha256hq_u32( d0, d1, T0 );  \
	d1 = vsha256h2q_u32( d1, d2, T0 );


void sha256_transform(uint32_t state[8], const uint32_t data[16], int swap)
{
	/* declare variables */
	uint32x4_t k0, k1, k2, k3, k4, k5, k6, k7, k8, k9, ka, kb, kc, kd, ke, kf;
	uint32x4_t s0, s1;
	uint32x4_t w0, w1, w2, w3;
	uint32x4_t d0, d1, d2;
	uint32x4_t t0, t1;

	/* set K0..Kf constants */
	k0 = vld1q_u32(&K[0x00]);
	k1 = vld1q_u32(&K[0x04]);
	k2 = vld1q_u32(&K[0x08]);
	k3 = vld1q_u32(&K[0x0c]);
	k4 = vld1q_u32(&K[0x10]);
	k5 = vld1q_u32(&K[0x14]);
	k6 = vld1q_u32(&K[0x18]);
	k7 = vld1q_u32(&K[0x1c]);
	k8 = vld1q_u32(&K[0x20]);
	k9 = vld1q_u32(&K[0x24]);
	ka = vld1q_u32(&K[0x28]);
	kb = vld1q_u32(&K[0x2c]);
	kc = vld1q_u32(&K[0x30]);
	kd = vld1q_u32(&K[0x34]);
	ke = vld1q_u32(&K[0x38]);
	kf = vld1q_u32(&K[0x3c]);

	/* load state */
	s0 = vld1q_u32(&state[0]);
	s1 = vld1q_u32(&state[4]);

	/* load message */
	w0 = vld1q_u32(data);
	w1 = vld1q_u32(data + 4);
	w2 = vld1q_u32(data + 8);
	w3 = vld1q_u32(data + 12);

	if (swap) {
		w0 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w0)));
		w1 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w1)));
		w2 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w2)));
		w3 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(w3)));
	}

	/* initialize t0, d0, d1 */
	t0 = vaddq_u32(w0, k0);
	d0 = s0;
	d1 = s1;

	/* perform rounds of four */
	Rx(t0, t1, k1, w0, w1, w2, w3);
	Rx(t1, t0, k2, w1, w2, w3, w0);
	Rx(t0, t1, k3, w2, w3, w0, w1);
	Rx(t1, t0, k4, w3, w0, w1, w2);
	Rx(t0, t1, k5, w0, w1, w2, w3);
	Rx(t1, t0, k6, w1, w2, w3, w0);
	Rx(t0, t1, k7, w2, w3, w0, w1);
	Rx(t1, t0, k8, w3, w0, w1, w2);
	Rx(t0, t1, k9, w0, w1, w2, w3);
	Rx(t1, t0, ka, w1, w2, w3, w0);
	Rx(t0, t1, kb, w2, w3, w0, w1);
	Rx(t1, t0, kc, w3, w0, w1, w2);
	Ry(t0, t1, kd, w1);
	Ry(t1, t0, ke, w2);
	Ry(t0, t1, kf, w3);
	Rz(t1);

	/* update state */
	s0 = vaddq_u32(s0, d0);
	s1 = vaddq_u32(s1, d1);

	/* save state */
	vst1q_u32(&state[0], s0);
	vst1q_u32(&state[4], s1);
}

static const uint32_t sha256d_hash1[16] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x80000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000100
};

static inline uint32_t be32dec(const void *pp)
{
	const uint8_t *p = (uint8_t const *)pp;
	return ((uint32_t)(p[3]) + ((uint32_t)(p[2]) << 8) +
	    ((uint32_t)(p[1]) << 16) + ((uint32_t)(p[0]) << 24));
}

static inline void be32enc(void *pp, uint32_t x)
{
	uint8_t *p = (uint8_t *)pp;
	p[3] = x & 0xff;
	p[2] = (x >> 8) & 0xff;
	p[1] = (x >> 16) & 0xff;
	p[0] = (x >> 24) & 0xff;
}

extern void sha256d(unsigned char *hash, const unsigned char *data, int len)
{
	uint32_t S[16], T[16];
	int i, r;

	sha256_init(S);
	for (r = len; r > -9; r -= 64) {
		if (r < 64)
			memset(T, 0, 64);
		memcpy(T, data + len - r, r > 64 ? 64 : (r < 0 ? 0 : r));
		if (r >= 0 && r < 64)
			((unsigned char *)T)[r] = 0x80;
		for (i = 0; i < 16; i++)
			T[i] = be32dec(T + i);
		if (r < 56)
			T[15] = 8 * len;
		sha256_transform(S, T, 0);
	}
	memcpy(S + 8, sha256d_hash1 + 8, 32);
	sha256_init(T);
	sha256_transform(T, S, 0);
	for (i = 0; i < 8; i++)
		be32enc((uint32_t *)hash + i, T[i]);
}

