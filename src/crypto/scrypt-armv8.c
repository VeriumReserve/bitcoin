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

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <compat/endian.h>

void sha256_init(uint32_t *state);
void sha256_transform(uint32_t *state, const uint32_t *block, int swap);
void scrypt_core(uint32_t* X, uint32_t* V, int N);

#define _ALIGN(x) __attribute__ ((aligned(x)))


#if defined(__aarch64__)

#include <arm_neon.h>

static inline void xor_salsa8(uint32_t B[16], const uint32_t Bx[16])
{
	uint32_t x00,x01,x02,x03,x04,x05,x06,x07,x08,x09,x10,x11,x12,x13,x14,x15;
	int i;

	x00 = (B[ 0] ^= Bx[ 0]);
	x01 = (B[ 1] ^= Bx[ 1]);
	x02 = (B[ 2] ^= Bx[ 2]);
	x03 = (B[ 3] ^= Bx[ 3]);
	x04 = (B[ 4] ^= Bx[ 4]);
	x05 = (B[ 5] ^= Bx[ 5]);
	x06 = (B[ 6] ^= Bx[ 6]);
	x07 = (B[ 7] ^= Bx[ 7]);
	x08 = (B[ 8] ^= Bx[ 8]);
	x09 = (B[ 9] ^= Bx[ 9]);
	x10 = (B[10] ^= Bx[10]);
	x11 = (B[11] ^= Bx[11]);
	x12 = (B[12] ^= Bx[12]);
	x13 = (B[13] ^= Bx[13]);
	x14 = (B[14] ^= Bx[14]);
	x15 = (B[15] ^= Bx[15]);
	for (i = 0; i < 8; i += 2) {
#define R(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
		/* Operate on columns. */
		x04 ^= R(x00+x12, 7);	x09 ^= R(x05+x01, 7);
		x14 ^= R(x10+x06, 7);	x03 ^= R(x15+x11, 7);
		
		x08 ^= R(x04+x00, 9);	x13 ^= R(x09+x05, 9);
		x02 ^= R(x14+x10, 9);	x07 ^= R(x03+x15, 9);
		
		x12 ^= R(x08+x04,13);	x01 ^= R(x13+x09,13);
		x06 ^= R(x02+x14,13);	x11 ^= R(x07+x03,13);
		
		x00 ^= R(x12+x08,18);	x05 ^= R(x01+x13,18);
		x10 ^= R(x06+x02,18);	x15 ^= R(x11+x07,18);
		
		/* Operate on rows. */
		x01 ^= R(x00+x03, 7);	x06 ^= R(x05+x04, 7);
		x11 ^= R(x10+x09, 7);	x12 ^= R(x15+x14, 7);
		
		x02 ^= R(x01+x00, 9);	x07 ^= R(x06+x05, 9);
		x08 ^= R(x11+x10, 9);	x13 ^= R(x12+x15, 9);
		
		x03 ^= R(x02+x01,13);	x04 ^= R(x07+x06,13);
		x09 ^= R(x08+x11,13);	x14 ^= R(x13+x12,13);
		
		x00 ^= R(x03+x02,18);	x05 ^= R(x04+x07,18);
		x10 ^= R(x09+x08,18);	x15 ^= R(x14+x13,18);
#undef R
	}
	B[ 0] += x00;
	B[ 1] += x01;
	B[ 2] += x02;
	B[ 3] += x03;
	B[ 4] += x04;
	B[ 5] += x05;
	B[ 6] += x06;
	B[ 7] += x07;
	B[ 8] += x08;
	B[ 9] += x09;
	B[10] += x10;
	B[11] += x11;
	B[12] += x12;
	B[13] += x13;
	B[14] += x14;
	B[15] += x15;
}


static inline void xor_salsa8_prefetch(uint32_t B[16], const uint32_t Bx[16], uint32_t* V, uint32_t N)
{
	uint32_t x00,x01,x02,x03,x04,x05,x06,x07,x08,x09,x10,x11,x12,x13,x14,x15;
	int i;

	x00 = (B[ 0] ^= Bx[ 0]);
	x01 = (B[ 1] ^= Bx[ 1]);
	x02 = (B[ 2] ^= Bx[ 2]);
	x03 = (B[ 3] ^= Bx[ 3]);
	x04 = (B[ 4] ^= Bx[ 4]);
	x05 = (B[ 5] ^= Bx[ 5]);
	x06 = (B[ 6] ^= Bx[ 6]);
	x07 = (B[ 7] ^= Bx[ 7]);
	x08 = (B[ 8] ^= Bx[ 8]);
	x09 = (B[ 9] ^= Bx[ 9]);
	x10 = (B[10] ^= Bx[10]);
	x11 = (B[11] ^= Bx[11]);
	x12 = (B[12] ^= Bx[12]);
	x13 = (B[13] ^= Bx[13]);
	x14 = (B[14] ^= Bx[14]);
	x15 = (B[15] ^= Bx[15]);
	for (i = 0; i < 8; i += 2) {
#define R(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
		/* Operate on columns. */
		x04 ^= R(x00+x12, 7);	x09 ^= R(x05+x01, 7);
		x14 ^= R(x10+x06, 7);	x03 ^= R(x15+x11, 7);
		
		x08 ^= R(x04+x00, 9);	x13 ^= R(x09+x05, 9);
		x02 ^= R(x14+x10, 9);	x07 ^= R(x03+x15, 9);
		
		x12 ^= R(x08+x04,13);	x01 ^= R(x13+x09,13);
		x06 ^= R(x02+x14,13);	x11 ^= R(x07+x03,13);
		
		x00 ^= R(x12+x08,18);	x05 ^= R(x01+x13,18);
		x10 ^= R(x06+x02,18);	x15 ^= R(x11+x07,18);
		
		/* Operate on rows. */
		x01 ^= R(x00+x03, 7);	x06 ^= R(x05+x04, 7);
		x11 ^= R(x10+x09, 7);	x12 ^= R(x15+x14, 7);
		
		x02 ^= R(x01+x00, 9);	x07 ^= R(x06+x05, 9);
		x08 ^= R(x11+x10, 9);	x13 ^= R(x12+x15, 9);
		
		x03 ^= R(x02+x01,13);	x04 ^= R(x07+x06,13);
		x09 ^= R(x08+x11,13);	x14 ^= R(x13+x12,13);
		
		x00 ^= R(x03+x02,18);	x05 ^= R(x04+x07,18);
		x10 ^= R(x09+x08,18);	x15 ^= R(x14+x13,18);
#undef R
	}
	B[ 0] += x00;
	uint32_t one = 32 * (B[0] & (N - 1));
	__builtin_prefetch(&V[one + 0]);
	__builtin_prefetch(&V[one + 8]);
	__builtin_prefetch(&V[one + 16]);
	__builtin_prefetch(&V[one + 24]);
	asm("":::"memory");
	B[ 1] += x01;
	B[ 2] += x02;
	B[ 3] += x03;
	B[ 4] += x04;
	B[ 5] += x05;
	B[ 6] += x06;
	B[ 7] += x07;
	B[ 8] += x08;
	B[ 9] += x09;
	B[10] += x10;
	B[11] += x11;
	B[12] += x12;
	B[13] += x13;
	B[14] += x14;
	B[15] += x15;
}

void scrypt_core(uint32_t *X, uint32_t *V, int N)
{
	int i;

	for (i = 0; i < N; i++) {
		memcpy(&V[i * 32], X, 128);
		xor_salsa8(&X[0], &X[16]);
		xor_salsa8(&X[16], &X[0]);
	}
	for (i = 0; i < N; i++) {
		uint32_t j = 32 * (X[16] & (N - 1));
		for (uint8_t k = 0; k < 32; k++)
			X[k] ^= V[j + k];
		xor_salsa8(&X[0], &X[16]);
		xor_salsa8_prefetch(&X[16], &X[0], V, N);
	}
}


static inline void scrypt_shuffle(uint32_t B[16])
{
	uint32_t x0 = 	B[0]; 
	uint32_t x1 = 	B[1]; 
	uint32_t x2 = 	B[2];
	uint32_t x3 = 	B[3]; 
	uint32_t x4 = 	B[4];
	uint32_t x5 = 	B[5]; 
	uint32_t x6 = 	B[6]; 
	uint32_t x7 = 	B[7]; 
	uint32_t x8 = 	B[8]; 
	uint32_t x9 = 	B[9]; 
	uint32_t x10 = B[10]; 
	uint32_t x11 = B[11]; 
	uint32_t x12 = B[12];
	uint32_t x13 = B[13]; 
	uint32_t x14 = B[14]; 
	uint32_t x15 = B[15]; 

	B[0] = x0;  B[1] = x5;  B[2] = x10;  B[3] = x15;
	B[4] = x12; B[5] = x1;  B[6] = x6;   B[7] = x11;
	B[8] = x8;  B[9] = x13; B[10] = x2;  B[11] = x7;
	B[12] = x4; B[13] = x9; B[14] = x14; B[15] = x3;
}

void scrypt_core_3way(uint32_t B[32 * 3], uint32_t *V, uint32_t N)
{
	uint32_t* W = V;

	scrypt_shuffle(&B[0  + 0]);
	scrypt_shuffle(&B[16 + 0]);
	scrypt_shuffle(&B[0 + 32]);
	scrypt_shuffle(&B[16 + 32]);
	scrypt_shuffle(&B[0 + 64]);
	scrypt_shuffle(&B[16 + 64]);

	uint32x4x4_t q_a, q_b, q_c, q_tmp;
	uint32x4x4_t ba_a, bb_a, bc_a, ba_b, bb_b, bc_b;

	ba_a.val[0] = vld1q_u32(&B[( 0) / 4]);
	ba_a.val[1] = vld1q_u32(&B[(16) / 4]);
	ba_a.val[2] = vld1q_u32(&B[(32) / 4]);
	ba_a.val[3] = vld1q_u32(&B[(48) / 4]);
	ba_b.val[0] = vld1q_u32(&B[(0 + 64 + 0) / 4]);
	ba_b.val[1] = vld1q_u32(&B[(0 + 64 + 16) / 4]);
	ba_b.val[2] = vld1q_u32(&B[(0 + 64 + 32) / 4]);
	ba_b.val[3] = vld1q_u32(&B[(0 + 64 + 48) / 4]);

	bb_a.val[0] = vld1q_u32(&B[(128 +  0) / 4]);
	bb_a.val[1] = vld1q_u32(&B[(128 + 16) / 4]);
	bb_a.val[2] = vld1q_u32(&B[(128 + 32) / 4]);
	bb_a.val[3] = vld1q_u32(&B[(128 + 48) / 4]);
	bb_b.val[0] = vld1q_u32(&B[(128 + 64 + 0) / 4]);
	bb_b.val[1] = vld1q_u32(&B[(128 + 64 + 16) / 4]);
	bb_b.val[2] = vld1q_u32(&B[(128 + 64 + 32) / 4]);
	bb_b.val[3] = vld1q_u32(&B[(128 + 64 + 48) / 4]);
	
	bc_a.val[0] = vld1q_u32(&B[(256 + 0) / 4]);
	bc_a.val[1] = vld1q_u32(&B[(256 + 16) / 4]);
	bc_a.val[2] = vld1q_u32(&B[(256 + 32) / 4]);
	bc_a.val[3] = vld1q_u32(&B[(256 + 48) / 4]);
	bc_b.val[0] = vld1q_u32(&B[(256 + 64 + 0) / 4]);
	bc_b.val[1] = vld1q_u32(&B[(256 + 64 + 16) / 4]);
	bc_b.val[2] = vld1q_u32(&B[(256 + 64 + 32) / 4]);
	bc_b.val[3] = vld1q_u32(&B[(256 + 64 + 48) / 4]);

	// prep

	vst1q_u32(&V[( 0) / 4], ba_a.val[0]);
	vst1q_u32(&V[(16) / 4], ba_a.val[1]);
	vst1q_u32(&V[(32) / 4], ba_a.val[2]);
	vst1q_u32(&V[(48) / 4], ba_a.val[3]);
	vst1q_u32(&V[(64) / 4],  ba_b.val[0]);
	vst1q_u32(&V[(80) / 4],  ba_b.val[1]);
	vst1q_u32(&V[(96) / 4],  ba_b.val[2]);
	vst1q_u32(&V[(112) / 4], ba_b.val[3]);

	vst1q_u32(&V[(128 +  0) / 4], bb_a.val[0]);
	vst1q_u32(&V[(128 + 16) / 4], bb_a.val[1]);
	vst1q_u32(&V[(128 + 32) / 4], bb_a.val[2]);
	vst1q_u32(&V[(128 + 48) / 4], bb_a.val[3]);
	vst1q_u32(&V[(128 + 64) / 4],  bb_b.val[0]);
	vst1q_u32(&V[(128 + 80) / 4],  bb_b.val[1]);
	vst1q_u32(&V[(128 + 96) / 4],  bb_b.val[2]);
	vst1q_u32(&V[(128 + 112) / 4], bb_b.val[3]);

	vst1q_u32(&V[(256 +  0) / 4], bc_a.val[0]);
	vst1q_u32(&V[(256 + 16) / 4], bc_a.val[1]);
	vst1q_u32(&V[(256 + 32) / 4], bc_a.val[2]);
	vst1q_u32(&V[(256 + 48) / 4], bc_a.val[3]);
	vst1q_u32(&V[(256 + 64) / 4], bc_b.val[0]);
	vst1q_u32(&V[(256 + 80) / 4], bc_b.val[1]);
	vst1q_u32(&V[(256 + 96) / 4], bc_b.val[2]);
	vst1q_u32(&V[(256 + 112) / 4],bc_b.val[3]);

	V += 96;

	for (int n = 0; n < N; n++)
	{
		// loop 1 part a
		q_a.val[0] = veorq_u32(ba_b.val[0], ba_a.val[0]);
		q_a.val[1] = veorq_u32(ba_b.val[1], ba_a.val[1]);
		q_a.val[2] = veorq_u32(ba_b.val[2], ba_a.val[2]);
		q_a.val[3] = veorq_u32(ba_b.val[3], ba_a.val[3]);

		q_b.val[0] = veorq_u32(bb_b.val[0], bb_a.val[0]);
		q_b.val[1] = veorq_u32(bb_b.val[1], bb_a.val[1]);
		q_b.val[2] = veorq_u32(bb_b.val[2], bb_a.val[2]);
		q_b.val[3] = veorq_u32(bb_b.val[3], bb_a.val[3]);

		q_c.val[0] = veorq_u32(bc_b.val[0], bc_a.val[0]);
		q_c.val[1] = veorq_u32(bc_b.val[1], bc_a.val[1]);
		q_c.val[2] = veorq_u32(bc_b.val[2], bc_a.val[2]);
		q_c.val[3] = veorq_u32(bc_b.val[3], bc_a.val[3]);

		ba_a = q_a;
		bb_a = q_b;
		bc_a = q_c;

		for (int i = 0; i < 4; i ++)
		{
			q_tmp.val[0] = vaddq_u32(q_a.val[0], q_a.val[1]);  	
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 7);	
			q_tmp.val[2] = vaddq_u32(q_b.val[0], q_b.val[1]);  	
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 25);
			q_a.val[3] = veorq_u32(q_tmp.val[1], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 7);
			q_tmp.val[3] = vaddq_u32(q_c.val[0], q_c.val[1]); 
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 25);
			q_b.val[3] = veorq_u32(q_tmp.val[1], q_b.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 7); 				
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 25);				
			q_c.val[3] = veorq_u32(q_tmp.val[1], q_c.val[3]);

			q_tmp.val[0] = vaddq_u32(q_a.val[3], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 9);
			q_tmp.val[2] = vaddq_u32(q_b.val[3], q_b.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 23);
			q_a.val[2] = veorq_u32(q_tmp.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 9);
			q_tmp.val[3] = vaddq_u32(q_c.val[3], q_c.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 23);
			q_b.val[2] = veorq_u32(q_tmp.val[1], q_b.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 9);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 23);
			q_c.val[2] = veorq_u32(q_tmp.val[1], q_c.val[2]);

			q_tmp.val[0] = vaddq_u32(q_a.val[2], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 13);
			q_tmp.val[2] = vaddq_u32(q_b.val[2], q_b.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 19);
			q_a.val[1] = veorq_u32(q_tmp.val[1], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 13);
			q_tmp.val[3] = vaddq_u32(q_c.val[2], q_c.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 19);
			q_b.val[1] = veorq_u32(q_tmp.val[1], q_b.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 13);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 19);
			q_c.val[1] = veorq_u32(q_tmp.val[1], q_c.val[1]);
			
			q_a.val[3] = vextq_u32(q_a.val[3], q_a.val[3], 3);
			q_b.val[3] = vextq_u32(q_b.val[3], q_b.val[3], 3);
			q_c.val[3] = vextq_u32(q_c.val[3], q_c.val[3], 3);
			
			q_tmp.val[0] = vaddq_u32(q_a.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 18);
			q_tmp.val[2] = vaddq_u32(q_b.val[1], q_b.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 14);
			q_a.val[0] = veorq_u32(q_tmp.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 18);
			q_tmp.val[3] = vaddq_u32(q_c.val[1], q_c.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 14);
			q_b.val[0] = veorq_u32(q_tmp.val[1], q_b.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 18);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 14);
			q_c.val[0] = veorq_u32(q_tmp.val[1], q_c.val[0]);
			
			q_a.val[2] = vextq_u32(q_a.val[2], q_a.val[2], 2);
			q_b.val[2] = vextq_u32(q_b.val[2], q_b.val[2], 2);
			q_c.val[2] = vextq_u32(q_c.val[2], q_c.val[2], 2);
			
			q_tmp.val[0] = vaddq_u32(q_a.val[0], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 7);
			q_tmp.val[2] = vaddq_u32(q_b.val[0], q_b.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 25);
			q_a.val[1] = vextq_u32(q_a.val[1], q_a.val[1], 1);
			q_a.val[1] = veorq_u32(q_tmp.val[1], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 7);
			q_tmp.val[3] = vaddq_u32(q_c.val[0], q_c.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 25);
			q_b.val[1] = vextq_u32(q_b.val[1], q_b.val[1], 1);
			q_b.val[1] = veorq_u32(q_tmp.val[1], q_b.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 7);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 25);
			q_c.val[1] = vextq_u32(q_c.val[1], q_c.val[1], 1);
			q_c.val[1] = veorq_u32(q_tmp.val[1], q_c.val[1]);

			q_tmp.val[0] = vaddq_u32(q_a.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 9);
			q_tmp.val[2] = vaddq_u32(q_b.val[1], q_b.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 23);
			q_a.val[2] = veorq_u32(q_tmp.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 9);
			q_tmp.val[3] = vaddq_u32(q_c.val[1], q_c.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 23);
			q_b.val[2] = veorq_u32(q_tmp.val[1], q_b.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 9);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 23);
			q_c.val[2] = veorq_u32(q_tmp.val[1], q_c.val[2]);

			q_tmp.val[0] = vaddq_u32(q_a.val[2], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 13);
			q_tmp.val[2] = vaddq_u32(q_b.val[2], q_b.val[1]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 19);
			q_a.val[3] = veorq_u32(q_tmp.val[1], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 13);
			q_tmp.val[3] = vaddq_u32(q_c.val[2], q_c.val[1]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 19);
			q_b.val[3] = veorq_u32(q_tmp.val[1], q_b.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 13);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 19);
			q_c.val[3] = veorq_u32(q_tmp.val[1], q_c.val[3]);
			q_a.val[1] = vextq_u32(q_a.val[1], q_a.val[1], 3);
			q_b.val[1] = vextq_u32(q_b.val[1], q_b.val[1], 3);
			q_c.val[1] = vextq_u32(q_c.val[1], q_c.val[1], 3);

			q_tmp.val[0] = vaddq_u32(q_a.val[3], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 18);
			q_tmp.val[2] = vaddq_u32(q_b.val[3], q_b.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 14);
			q_a.val[2] = vextq_u32(q_a.val[2], q_a.val[2], 2);
			q_b.val[2] = vextq_u32(q_b.val[2], q_b.val[2], 2);
			q_a.val[0] = veorq_u32(q_tmp.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 18);
			q_tmp.val[3] = vaddq_u32(q_c.val[3], q_c.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 14);
			q_c.val[2] = vextq_u32(q_c.val[2], q_c.val[2], 2);
			q_b.val[3] = vextq_u32(q_b.val[3], q_b.val[3], 1);
			q_b.val[0] = veorq_u32(q_tmp.val[1], q_b.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 18);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 14);
			q_a.val[3] = vextq_u32(q_a.val[3], q_a.val[3], 1);
			q_c.val[3] = vextq_u32(q_c.val[3], q_c.val[3], 1);
			q_c.val[0] = veorq_u32(q_tmp.val[1], q_c.val[0]);
		}
		ba_a.val[0] = vaddq_u32(ba_a.val[0], q_a.val[0]);
		ba_a.val[1] = vaddq_u32(ba_a.val[1], q_a.val[1]);
		ba_a.val[2] = vaddq_u32(ba_a.val[2], q_a.val[2]);
		ba_a.val[3] = vaddq_u32(ba_a.val[3], q_a.val[3]);

		q_a = ba_a;

		bb_a.val[0] = vaddq_u32(bb_a.val[0], q_b.val[0]);
		bb_a.val[1] = vaddq_u32(bb_a.val[1], q_b.val[1]);
		bb_a.val[2] = vaddq_u32(bb_a.val[2], q_b.val[2]);
		bb_a.val[3] = vaddq_u32(bb_a.val[3], q_b.val[3]);

		q_b = bb_a;

		bc_a.val[0] = vaddq_u32(bc_a.val[0], q_c.val[0]);
		bc_a.val[1] = vaddq_u32(bc_a.val[1], q_c.val[1]);
		bc_a.val[2] = vaddq_u32(bc_a.val[2], q_c.val[2]);
		bc_a.val[3] = vaddq_u32(bc_a.val[3], q_c.val[3]);

		q_c = bc_a;
		
		for (int i = 0; i < 4; i++)
		{
			vst1q_u32(&V[      (i * 4) ], ba_a.val[i]);
			vst1q_u32(&V[(32 + (i * 4))], bb_a.val[i]);
			vst1q_u32(&V[(64 + (i * 4))], bc_a.val[i]);
		}

		// loop 1 part b

		q_a.val[0] = veorq_u32(ba_b.val[0], q_a.val[0]);
		q_a.val[1] = veorq_u32(ba_b.val[1], q_a.val[1]);
		q_a.val[2] = veorq_u32(ba_b.val[2], q_a.val[2]);
		q_a.val[3] = veorq_u32(ba_b.val[3], q_a.val[3]);
		ba_b = q_a;

		q_b.val[0] = veorq_u32(bb_b.val[0], q_b.val[0]);
		q_b.val[1] = veorq_u32(bb_b.val[1], q_b.val[1]);
		q_b.val[2] = veorq_u32(bb_b.val[2], q_b.val[2]);
		q_b.val[3] = veorq_u32(bb_b.val[3], q_b.val[3]);
		bb_b = q_b;

		q_c.val[0] = veorq_u32(bc_b.val[0], q_c.val[0]);
		q_c.val[1] = veorq_u32(bc_b.val[1], q_c.val[1]);
		q_c.val[2] = veorq_u32(bc_b.val[2], q_c.val[2]);
		q_c.val[3] = veorq_u32(bc_b.val[3], q_c.val[3]);
		bc_b = q_c;


		for (int i = 0; i < 4; i ++)
		{
			q_tmp.val[0] = vaddq_u32(q_a.val[0], q_a.val[1]);  	
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 7);	
			q_tmp.val[2] = vaddq_u32(q_b.val[0], q_b.val[1]);  	
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 25);
			q_a.val[3] = veorq_u32(q_tmp.val[1], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 7);
			q_tmp.val[3] = vaddq_u32(q_c.val[0], q_c.val[1]); 
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 25);
			q_b.val[3] = veorq_u32(q_tmp.val[1], q_b.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 7); 				
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 25);				
			q_c.val[3] = veorq_u32(q_tmp.val[1], q_c.val[3]);

			q_tmp.val[0] = vaddq_u32(q_a.val[3], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 9);
			q_tmp.val[2] = vaddq_u32(q_b.val[3], q_b.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 23);
			q_a.val[2] = veorq_u32(q_tmp.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 9);
			q_tmp.val[3] = vaddq_u32(q_c.val[3], q_c.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 23);
			q_b.val[2] = veorq_u32(q_tmp.val[1], q_b.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 9);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 23);
			q_c.val[2] = veorq_u32(q_tmp.val[1], q_c.val[2]);

			q_tmp.val[0] = vaddq_u32(q_a.val[2], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 13);
			q_tmp.val[2] = vaddq_u32(q_b.val[2], q_b.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 19);
			q_a.val[1] = veorq_u32(q_tmp.val[1], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 13);
			q_tmp.val[3] = vaddq_u32(q_c.val[2], q_c.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 19);
			q_b.val[1] = veorq_u32(q_tmp.val[1], q_b.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 13);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 19);
			q_c.val[1] = veorq_u32(q_tmp.val[1], q_c.val[1]);
			
			q_a.val[3] = vextq_u32(q_a.val[3], q_a.val[3], 3);
			q_b.val[3] = vextq_u32(q_b.val[3], q_b.val[3], 3);
			q_c.val[3] = vextq_u32(q_c.val[3], q_c.val[3], 3);
			
			q_tmp.val[0] = vaddq_u32(q_a.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 18);
			q_tmp.val[2] = vaddq_u32(q_b.val[1], q_b.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 14);
			q_a.val[0] = veorq_u32(q_tmp.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 18);
			q_tmp.val[3] = vaddq_u32(q_c.val[1], q_c.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 14);
			q_b.val[0] = veorq_u32(q_tmp.val[1], q_b.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 18);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 14);
			q_c.val[0] = veorq_u32(q_tmp.val[1], q_c.val[0]);
			
			q_a.val[2] = vextq_u32(q_a.val[2], q_a.val[2], 2);
			q_b.val[2] = vextq_u32(q_b.val[2], q_b.val[2], 2);
			q_c.val[2] = vextq_u32(q_c.val[2], q_c.val[2], 2);
			
			q_tmp.val[0] = vaddq_u32(q_a.val[0], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 7);
			q_tmp.val[2] = vaddq_u32(q_b.val[0], q_b.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 25);
			q_a.val[1] = vextq_u32(q_a.val[1], q_a.val[1], 1);
			q_a.val[1] = veorq_u32(q_tmp.val[1], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 7);
			q_tmp.val[3] = vaddq_u32(q_c.val[0], q_c.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 25);
			q_b.val[1] = vextq_u32(q_b.val[1], q_b.val[1], 1);
			q_b.val[1] = veorq_u32(q_tmp.val[1], q_b.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 7);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 25);
			q_c.val[1] = vextq_u32(q_c.val[1], q_c.val[1], 1);
			q_c.val[1] = veorq_u32(q_tmp.val[1], q_c.val[1]);

			q_tmp.val[0] = vaddq_u32(q_a.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 9);
			q_tmp.val[2] = vaddq_u32(q_b.val[1], q_b.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 23);
			q_a.val[2] = veorq_u32(q_tmp.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 9);
			q_tmp.val[3] = vaddq_u32(q_c.val[1], q_c.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 23);
			q_b.val[2] = veorq_u32(q_tmp.val[1], q_b.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 9);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 23);
			q_c.val[2] = veorq_u32(q_tmp.val[1], q_c.val[2]);

			q_tmp.val[0] = vaddq_u32(q_a.val[2], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 13);
			q_tmp.val[2] = vaddq_u32(q_b.val[2], q_b.val[1]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 19);
			q_a.val[3] = veorq_u32(q_tmp.val[1], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 13);
			q_tmp.val[3] = vaddq_u32(q_c.val[2], q_c.val[1]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 19);
			q_b.val[3] = veorq_u32(q_tmp.val[1], q_b.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 13);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 19);
			q_c.val[3] = veorq_u32(q_tmp.val[1], q_c.val[3]);
			q_a.val[1] = vextq_u32(q_a.val[1], q_a.val[1], 3);
			q_b.val[1] = vextq_u32(q_b.val[1], q_b.val[1], 3);
			q_c.val[1] = vextq_u32(q_c.val[1], q_c.val[1], 3);

			q_tmp.val[0] = vaddq_u32(q_a.val[3], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 18);
			q_tmp.val[2] = vaddq_u32(q_b.val[3], q_b.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 14);
			q_a.val[2] = vextq_u32(q_a.val[2], q_a.val[2], 2);
			q_b.val[2] = vextq_u32(q_b.val[2], q_b.val[2], 2);
			q_a.val[0] = veorq_u32(q_tmp.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 18);
			q_tmp.val[3] = vaddq_u32(q_c.val[3], q_c.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 14);
			q_c.val[2] = vextq_u32(q_c.val[2], q_c.val[2], 2);
			q_b.val[3] = vextq_u32(q_b.val[3], q_b.val[3], 1);
			q_b.val[0] = veorq_u32(q_tmp.val[1], q_b.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 18);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 14);
			q_a.val[3] = vextq_u32(q_a.val[3], q_a.val[3], 1);
			q_c.val[3] = vextq_u32(q_c.val[3], q_c.val[3], 1);
			q_c.val[0] = veorq_u32(q_tmp.val[1], q_c.val[0]);
		}

		ba_b.val[0] = vaddq_u32(q_a.val[0], ba_b.val[0]);
		ba_b.val[1] = vaddq_u32(q_a.val[1], ba_b.val[1]);
		ba_b.val[2] = vaddq_u32(q_a.val[2], ba_b.val[2]);
		ba_b.val[3] = vaddq_u32(q_a.val[3], ba_b.val[3]);
		bb_b.val[0] = vaddq_u32(q_b.val[0], bb_b.val[0]);
		bb_b.val[1] = vaddq_u32(q_b.val[1], bb_b.val[1]);
		bb_b.val[2] = vaddq_u32(q_b.val[2], bb_b.val[2]);
		bb_b.val[3] = vaddq_u32(q_b.val[3], bb_b.val[3]);
		bc_b.val[0] = vaddq_u32(q_c.val[0], bc_b.val[0]);
		bc_b.val[1] = vaddq_u32(q_c.val[1], bc_b.val[1]);
		bc_b.val[2] = vaddq_u32(q_c.val[2], bc_b.val[2]);
		bc_b.val[3] = vaddq_u32(q_c.val[3], bc_b.val[3]);
		for (int i = 0; i < 4; i++)
		{
			vst1q_u32(&V[(     16 + (i * 4))], ba_b.val[i]);
			vst1q_u32(&V[(32 + 16 + (i * 4))], bb_b.val[i]);
			vst1q_u32(&V[(64 + 16 + (i * 4))], bc_b.val[i]);
		}
		V += 96;
	}
	V = W;

    // loop 2

	uint32x4x4_t x;

	uint32_t one =   32 * (3 * (ba_b.val[0][0] & (N - 1)) + 0);
	uint32_t two =   32 * (3 * (bb_b.val[0][0] & (N - 1)) + 1);
	uint32_t three = 32 * (3 * (bc_b.val[0][0] & (N - 1)) + 2);
	x.val[0] = vld1q_u32(&W[one +  0]);
	x.val[1] = vld1q_u32(&W[one +  4]);
	x.val[2] = vld1q_u32(&W[one +  8]);
	x.val[3] = vld1q_u32(&W[one + 12]);

	for (int n = 0; n < N; n++)
	{
		// loop 2 part a

		ba_a.val[0] = veorq_u32(ba_a.val[0], x.val[0]);
			x.val[0] = vld1q_u32(&W[one + 16 +  0]);
		ba_a.val[1] = veorq_u32(ba_a.val[1], x.val[1]);
			x.val[1] = vld1q_u32(&W[one + 16 +  4]);
		ba_a.val[2] = veorq_u32(ba_a.val[2], x.val[2]);
			x.val[2] = vld1q_u32(&W[one + 16 +  8]);
		ba_a.val[3] = veorq_u32(ba_a.val[3], x.val[3]);

			ba_b.val[0] = veorq_u32(ba_b.val[0], x.val[0]);
			ba_b.val[1] = veorq_u32(ba_b.val[1], x.val[1]);
			x.val[3] = vld1q_u32(&W[one + 16 + 12]);
			ba_b.val[2] = veorq_u32(ba_b.val[2], x.val[2]);
			ba_b.val[3] = veorq_u32(ba_b.val[3], x.val[3]);
		x.val[0] = vld1q_u32(&W[two +  0]);
				q_a.val[0] = veorq_u32(ba_b.val[0], ba_a.val[0]);
				q_a.val[1] = veorq_u32(ba_b.val[1], ba_a.val[1]);
		x.val[1] = vld1q_u32(&W[two +  4]);
				q_a.val[2] = veorq_u32(ba_b.val[2], ba_a.val[2]);
				q_a.val[3] = veorq_u32(ba_b.val[3], ba_a.val[3]);
		x.val[2] = vld1q_u32(&W[two +  8]);
		ba_a = q_a;

		x.val[3] = vld1q_u32(&W[two + 12]);

		bb_a.val[0] = veorq_u32(bb_a.val[0], x.val[0]);
			x.val[0] = vld1q_u32(&W[two + 16 +  0]);
		bb_a.val[1] = veorq_u32(bb_a.val[1], x.val[1]);
			x.val[1] = vld1q_u32(&W[two + 16 +  4]);
		bb_a.val[2] = veorq_u32(bb_a.val[2], x.val[2]);
			x.val[2] = vld1q_u32(&W[two + 16 +  8]);
		bb_a.val[3] = veorq_u32(bb_a.val[3], x.val[3]);
			bb_b.val[0] = veorq_u32(bb_b.val[0], x.val[0]);
			x.val[3] = vld1q_u32(&W[two + 16 + 12]);
			bb_b.val[1] = veorq_u32(bb_b.val[1], x.val[1]);
		x.val[0] = vld1q_u32(&W[three +  0]);
			bb_b.val[2] = veorq_u32(bb_b.val[2], x.val[2]);
			bb_b.val[3] = veorq_u32(bb_b.val[3], x.val[3]);
		x.val[1] = vld1q_u32(&W[three +  4]);
				q_b.val[0] = veorq_u32(bb_b.val[0], bb_a.val[0]);
				q_b.val[1] = veorq_u32(bb_b.val[1], bb_a.val[1]);
		x.val[2] = vld1q_u32(&W[three +  8]);
				q_b.val[2] = veorq_u32(bb_b.val[2], bb_a.val[2]);
				q_b.val[3] = veorq_u32(bb_b.val[3], bb_a.val[3]);
		x.val[3] = vld1q_u32(&W[three + 12]);
		bb_a = q_b;

		bc_a.val[0] = veorq_u32(bc_a.val[0], x.val[0]);
			x.val[0] = vld1q_u32(&W[three + 16 +  0]);
		bc_a.val[1] = veorq_u32(bc_a.val[1], x.val[1]);
			x.val[1] = vld1q_u32(&W[three + 16 +  4]);
		bc_a.val[2] = veorq_u32(bc_a.val[2], x.val[2]);
			x.val[2] = vld1q_u32(&W[three + 16 +  8]);
		bc_a.val[3] = veorq_u32(bc_a.val[3], x.val[3]);
			bc_b.val[0] = veorq_u32(bc_b.val[0], x.val[0]);
			x.val[3] = vld1q_u32(&W[three + 16 + 12]);
			bc_b.val[1] = veorq_u32(bc_b.val[1], x.val[1]);
			bc_b.val[2] = veorq_u32(bc_b.val[2], x.val[2]);
			bc_b.val[3] = veorq_u32(bc_b.val[3], x.val[3]);
				q_c.val[0] = veorq_u32(bc_b.val[0], bc_a.val[0]);
				q_c.val[1] = veorq_u32(bc_b.val[1], bc_a.val[1]);
				q_c.val[2] = veorq_u32(bc_b.val[2], bc_a.val[2]);
				q_c.val[3] = veorq_u32(bc_b.val[3], bc_a.val[3]);
		bc_a = q_c;

		for (int i = 0; i < 4; i++)
		{
			q_tmp.val[0] = vaddq_u32(q_a.val[0], q_a.val[1]);  	
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 7);	
			q_tmp.val[2] = vaddq_u32(q_b.val[0], q_b.val[1]);  	
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 25);
			q_a.val[3] = veorq_u32(q_tmp.val[1], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 7);
			q_tmp.val[3] = vaddq_u32(q_c.val[0], q_c.val[1]); 
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 25);
			q_b.val[3] = veorq_u32(q_tmp.val[1], q_b.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 7); 				
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 25);				
			q_c.val[3] = veorq_u32(q_tmp.val[1], q_c.val[3]);

			q_tmp.val[0] = vaddq_u32(q_a.val[3], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 9);
			q_tmp.val[2] = vaddq_u32(q_b.val[3], q_b.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 23);
			q_a.val[2] = veorq_u32(q_tmp.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 9);
			q_tmp.val[3] = vaddq_u32(q_c.val[3], q_c.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 23);
			q_b.val[2] = veorq_u32(q_tmp.val[1], q_b.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 9);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 23);
			q_c.val[2] = veorq_u32(q_tmp.val[1], q_c.val[2]);

			q_tmp.val[0] = vaddq_u32(q_a.val[2], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 13);
			q_tmp.val[2] = vaddq_u32(q_b.val[2], q_b.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 19);
			q_a.val[1] = veorq_u32(q_tmp.val[1], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 13);
			q_tmp.val[3] = vaddq_u32(q_c.val[2], q_c.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 19);
			q_b.val[1] = veorq_u32(q_tmp.val[1], q_b.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 13);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 19);
			q_c.val[1] = veorq_u32(q_tmp.val[1], q_c.val[1]);
			
			q_a.val[3] = vextq_u32(q_a.val[3], q_a.val[3], 3);
			q_b.val[3] = vextq_u32(q_b.val[3], q_b.val[3], 3);
			q_c.val[3] = vextq_u32(q_c.val[3], q_c.val[3], 3);
			
			q_tmp.val[0] = vaddq_u32(q_a.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 18);
			q_tmp.val[2] = vaddq_u32(q_b.val[1], q_b.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 14);
			q_a.val[0] = veorq_u32(q_tmp.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 18);
			q_tmp.val[3] = vaddq_u32(q_c.val[1], q_c.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 14);
			q_b.val[0] = veorq_u32(q_tmp.val[1], q_b.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 18);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 14);
			q_c.val[0] = veorq_u32(q_tmp.val[1], q_c.val[0]);
			
			q_a.val[2] = vextq_u32(q_a.val[2], q_a.val[2], 2);
			q_b.val[2] = vextq_u32(q_b.val[2], q_b.val[2], 2);
			q_c.val[2] = vextq_u32(q_c.val[2], q_c.val[2], 2);
			
			q_tmp.val[0] = vaddq_u32(q_a.val[0], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 7);
			q_tmp.val[2] = vaddq_u32(q_b.val[0], q_b.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 25);
			q_a.val[1] = vextq_u32(q_a.val[1], q_a.val[1], 1);
			q_a.val[1] = veorq_u32(q_tmp.val[1], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 7);
			q_tmp.val[3] = vaddq_u32(q_c.val[0], q_c.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 25);
			q_b.val[1] = vextq_u32(q_b.val[1], q_b.val[1], 1);
			q_b.val[1] = veorq_u32(q_tmp.val[1], q_b.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 7);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 25);
			q_c.val[1] = vextq_u32(q_c.val[1], q_c.val[1], 1);
			q_c.val[1] = veorq_u32(q_tmp.val[1], q_c.val[1]);

			q_tmp.val[0] = vaddq_u32(q_a.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 9);
			q_tmp.val[2] = vaddq_u32(q_b.val[1], q_b.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 23);
			q_a.val[2] = veorq_u32(q_tmp.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 9);
			q_tmp.val[3] = vaddq_u32(q_c.val[1], q_c.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 23);
			q_b.val[2] = veorq_u32(q_tmp.val[1], q_b.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 9);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 23);
			q_c.val[2] = veorq_u32(q_tmp.val[1], q_c.val[2]);

			q_tmp.val[0] = vaddq_u32(q_a.val[2], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 13);
			q_tmp.val[2] = vaddq_u32(q_b.val[2], q_b.val[1]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 19);
			q_a.val[3] = veorq_u32(q_tmp.val[1], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 13);
			q_tmp.val[3] = vaddq_u32(q_c.val[2], q_c.val[1]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 19);
			q_b.val[3] = veorq_u32(q_tmp.val[1], q_b.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 13);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 19);
			q_c.val[3] = veorq_u32(q_tmp.val[1], q_c.val[3]);
			q_a.val[1] = vextq_u32(q_a.val[1], q_a.val[1], 3);
			q_b.val[1] = vextq_u32(q_b.val[1], q_b.val[1], 3);
			q_c.val[1] = vextq_u32(q_c.val[1], q_c.val[1], 3);

			q_tmp.val[0] = vaddq_u32(q_a.val[3], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 18);
			q_tmp.val[2] = vaddq_u32(q_b.val[3], q_b.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 14);
			q_a.val[2] = vextq_u32(q_a.val[2], q_a.val[2], 2);
			q_b.val[2] = vextq_u32(q_b.val[2], q_b.val[2], 2);
			q_a.val[0] = veorq_u32(q_tmp.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 18);
			q_tmp.val[3] = vaddq_u32(q_c.val[3], q_c.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 14);
			q_c.val[2] = vextq_u32(q_c.val[2], q_c.val[2], 2);
			q_b.val[3] = vextq_u32(q_b.val[3], q_b.val[3], 1);
			q_b.val[0] = veorq_u32(q_tmp.val[1], q_b.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 18);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 14);
			q_a.val[3] = vextq_u32(q_a.val[3], q_a.val[3], 1);
			q_c.val[3] = vextq_u32(q_c.val[3], q_c.val[3], 1);
			q_c.val[0] = veorq_u32(q_tmp.val[1], q_c.val[0]);
		}
		ba_a.val[0] = vaddq_u32(ba_a.val[0], q_a.val[0]);
		ba_a.val[1] = vaddq_u32(ba_a.val[1], q_a.val[1]);
		ba_a.val[2] = vaddq_u32(ba_a.val[2], q_a.val[2]);
		ba_a.val[3] = vaddq_u32(ba_a.val[3], q_a.val[3]);

		q_a = ba_a;

		bb_a.val[0] = vaddq_u32(bb_a.val[0], q_b.val[0]);
		bb_a.val[1] = vaddq_u32(bb_a.val[1], q_b.val[1]);
		bb_a.val[2] = vaddq_u32(bb_a.val[2], q_b.val[2]);
		bb_a.val[3] = vaddq_u32(bb_a.val[3], q_b.val[3]);
		q_b = bb_a;

		bc_a.val[0] = vaddq_u32(bc_a.val[0], q_c.val[0]);
		bc_a.val[1] = vaddq_u32(bc_a.val[1], q_c.val[1]);
		bc_a.val[2] = vaddq_u32(bc_a.val[2], q_c.val[2]);
		bc_a.val[3] = vaddq_u32(bc_a.val[3], q_c.val[3]);
		q_c = bc_a;

		// loop 2 b

		q_a.val[0] = veorq_u32(ba_b.val[0], q_a.val[0]);
		q_a.val[1] = veorq_u32(ba_b.val[1], q_a.val[1]);
		q_a.val[2] = veorq_u32(ba_b.val[2], q_a.val[2]);
		q_a.val[3] = veorq_u32(ba_b.val[3], q_a.val[3]);
		ba_b = q_a;

		q_b.val[0] = veorq_u32(bb_b.val[0], q_b.val[0]);
		q_b.val[1] = veorq_u32(bb_b.val[1], q_b.val[1]);
		q_b.val[2] = veorq_u32(bb_b.val[2], q_b.val[2]);
		q_b.val[3] = veorq_u32(bb_b.val[3], q_b.val[3]);
		bb_b = q_b;

		q_c.val[0] = veorq_u32(bc_b.val[0], q_c.val[0]);
		q_c.val[1] = veorq_u32(bc_b.val[1], q_c.val[1]);
		q_c.val[2] = veorq_u32(bc_b.val[2], q_c.val[2]);
		q_c.val[3] = veorq_u32(bc_b.val[3], q_c.val[3]);
		bc_b = q_c;


		for (int i = 0; i < 3; i++)
		{
			q_tmp.val[0] = vaddq_u32(q_a.val[0], q_a.val[1]);  	
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 7);	
			q_tmp.val[2] = vaddq_u32(q_b.val[0], q_b.val[1]);  	
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 25);
			q_a.val[3] = veorq_u32(q_tmp.val[1], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 7);
			q_tmp.val[3] = vaddq_u32(q_c.val[0], q_c.val[1]); 
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 25);
			q_b.val[3] = veorq_u32(q_tmp.val[1], q_b.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 7); 				
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 25);				
			q_c.val[3] = veorq_u32(q_tmp.val[1], q_c.val[3]);

			q_tmp.val[0] = vaddq_u32(q_a.val[3], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 9);
			q_tmp.val[2] = vaddq_u32(q_b.val[3], q_b.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 23);
			q_a.val[2] = veorq_u32(q_tmp.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 9);
			q_tmp.val[3] = vaddq_u32(q_c.val[3], q_c.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 23);
			q_b.val[2] = veorq_u32(q_tmp.val[1], q_b.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 9);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 23);
			q_c.val[2] = veorq_u32(q_tmp.val[1], q_c.val[2]);

			q_tmp.val[0] = vaddq_u32(q_a.val[2], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 13);
			q_tmp.val[2] = vaddq_u32(q_b.val[2], q_b.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 19);
			q_a.val[1] = veorq_u32(q_tmp.val[1], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 13);
			q_tmp.val[3] = vaddq_u32(q_c.val[2], q_c.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 19);
			q_b.val[1] = veorq_u32(q_tmp.val[1], q_b.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 13);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 19);
			q_c.val[1] = veorq_u32(q_tmp.val[1], q_c.val[1]);
			
			q_a.val[3] = vextq_u32(q_a.val[3], q_a.val[3], 3);
			q_b.val[3] = vextq_u32(q_b.val[3], q_b.val[3], 3);
			q_c.val[3] = vextq_u32(q_c.val[3], q_c.val[3], 3);
			
			q_tmp.val[0] = vaddq_u32(q_a.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 18);
			q_tmp.val[2] = vaddq_u32(q_b.val[1], q_b.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 14);
			q_a.val[0] = veorq_u32(q_tmp.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 18);
			q_tmp.val[3] = vaddq_u32(q_c.val[1], q_c.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 14);
			q_b.val[0] = veorq_u32(q_tmp.val[1], q_b.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 18);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 14);
			q_c.val[0] = veorq_u32(q_tmp.val[1], q_c.val[0]);
			
			q_a.val[2] = vextq_u32(q_a.val[2], q_a.val[2], 2);
			q_b.val[2] = vextq_u32(q_b.val[2], q_b.val[2], 2);
			q_c.val[2] = vextq_u32(q_c.val[2], q_c.val[2], 2);
			
			q_tmp.val[0] = vaddq_u32(q_a.val[0], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 7);
			q_tmp.val[2] = vaddq_u32(q_b.val[0], q_b.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 25);
			q_a.val[1] = vextq_u32(q_a.val[1], q_a.val[1], 1);
			q_a.val[1] = veorq_u32(q_tmp.val[1], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 7);
			q_tmp.val[3] = vaddq_u32(q_c.val[0], q_c.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 25);
			q_b.val[1] = vextq_u32(q_b.val[1], q_b.val[1], 1);
			q_b.val[1] = veorq_u32(q_tmp.val[1], q_b.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 7);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 25);
			q_c.val[1] = vextq_u32(q_c.val[1], q_c.val[1], 1);
			q_c.val[1] = veorq_u32(q_tmp.val[1], q_c.val[1]);

			q_tmp.val[0] = vaddq_u32(q_a.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 9);
			q_tmp.val[2] = vaddq_u32(q_b.val[1], q_b.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 23);
			q_a.val[2] = veorq_u32(q_tmp.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 9);
			q_tmp.val[3] = vaddq_u32(q_c.val[1], q_c.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 23);
			q_b.val[2] = veorq_u32(q_tmp.val[1], q_b.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 9);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 23);
			q_c.val[2] = veorq_u32(q_tmp.val[1], q_c.val[2]);

			q_tmp.val[0] = vaddq_u32(q_a.val[2], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 13);
			q_tmp.val[2] = vaddq_u32(q_b.val[2], q_b.val[1]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 19);
			q_a.val[3] = veorq_u32(q_tmp.val[1], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 13);
			q_tmp.val[3] = vaddq_u32(q_c.val[2], q_c.val[1]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 19);
			q_b.val[3] = veorq_u32(q_tmp.val[1], q_b.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 13);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 19);
			q_c.val[3] = veorq_u32(q_tmp.val[1], q_c.val[3]);
			q_a.val[1] = vextq_u32(q_a.val[1], q_a.val[1], 3);
			q_b.val[1] = vextq_u32(q_b.val[1], q_b.val[1], 3);
			q_c.val[1] = vextq_u32(q_c.val[1], q_c.val[1], 3);

			q_tmp.val[0] = vaddq_u32(q_a.val[3], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 18);
			q_tmp.val[2] = vaddq_u32(q_b.val[3], q_b.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 14);
			q_a.val[2] = vextq_u32(q_a.val[2], q_a.val[2], 2);
			q_b.val[2] = vextq_u32(q_b.val[2], q_b.val[2], 2);
			q_a.val[0] = veorq_u32(q_tmp.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 18);
			q_tmp.val[3] = vaddq_u32(q_c.val[3], q_c.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 14);
			q_c.val[2] = vextq_u32(q_c.val[2], q_c.val[2], 2);
			q_b.val[3] = vextq_u32(q_b.val[3], q_b.val[3], 1);
			q_b.val[0] = veorq_u32(q_tmp.val[1], q_b.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 18);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 14);
			q_a.val[3] = vextq_u32(q_a.val[3], q_a.val[3], 1);
			q_c.val[3] = vextq_u32(q_c.val[3], q_c.val[3], 1);
			q_c.val[0] = veorq_u32(q_tmp.val[1], q_c.val[0]);
		}
		{
			//1
			q_tmp.val[0] = vaddq_u32(q_a.val[0], q_a.val[1]);  	
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 7);	
			q_tmp.val[2] = vaddq_u32(q_b.val[0], q_b.val[1]);  	
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 25);
			q_a.val[3] = veorq_u32(q_tmp.val[1], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 7);
			q_tmp.val[3] = vaddq_u32(q_c.val[0], q_c.val[1]); 
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 25);
			q_b.val[3] = veorq_u32(q_tmp.val[1], q_b.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 7); 				
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 25);				
			q_c.val[3] = veorq_u32(q_tmp.val[1], q_c.val[3]);
			//2
			q_tmp.val[0] = vaddq_u32(q_a.val[3], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 9);
			q_tmp.val[2] = vaddq_u32(q_b.val[3], q_b.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 23);
			q_a.val[2] = veorq_u32(q_tmp.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 9);
			q_tmp.val[3] = vaddq_u32(q_c.val[3], q_c.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 23);
			q_b.val[2] = veorq_u32(q_tmp.val[1], q_b.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 9);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 23);
			q_c.val[2] = veorq_u32(q_tmp.val[1], q_c.val[2]);
			//3
			q_tmp.val[0] = vaddq_u32(q_a.val[2], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 13);
			q_tmp.val[2] = vaddq_u32(q_b.val[2], q_b.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 19);
			q_a.val[1] = veorq_u32(q_tmp.val[1], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 13);
			q_tmp.val[3] = vaddq_u32(q_c.val[2], q_c.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 19);
			q_b.val[1] = veorq_u32(q_tmp.val[1], q_b.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 13);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 19);
			q_c.val[1] = veorq_u32(q_tmp.val[1], q_c.val[1]);
			
			q_a.val[3] = vextq_u32(q_a.val[3], q_a.val[3], 3);
			q_b.val[3] = vextq_u32(q_b.val[3], q_b.val[3], 3);
			q_c.val[3] = vextq_u32(q_c.val[3], q_c.val[3], 3);
			//4
			q_tmp.val[0] = vaddq_u32(q_a.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 18);
			q_tmp.val[2] = vaddq_u32(q_b.val[1], q_b.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 14);
			q_a.val[0] = veorq_u32(q_tmp.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 18);
			q_tmp.val[3] = vaddq_u32(q_c.val[1], q_c.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 14);
			q_b.val[0] = veorq_u32(q_tmp.val[1], q_b.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 18);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 14);
			q_c.val[0] = veorq_u32(q_tmp.val[1], q_c.val[0]);
			
			q_a.val[2] = vextq_u32(q_a.val[2], q_a.val[2], 2);
			q_b.val[2] = vextq_u32(q_b.val[2], q_b.val[2], 2);
			q_c.val[2] = vextq_u32(q_c.val[2], q_c.val[2], 2);
			//5
			q_tmp.val[0] = vaddq_u32(q_a.val[0], q_a.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 7);
			q_tmp.val[2] = vaddq_u32(q_b.val[0], q_b.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 25);
			q_a.val[1] = vextq_u32(q_a.val[1], q_a.val[1], 1);
			q_a.val[1] = veorq_u32(q_tmp.val[1], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 7);
			q_tmp.val[3] = vaddq_u32(q_c.val[0], q_c.val[3]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 25);
			q_b.val[1] = vextq_u32(q_b.val[1], q_b.val[1], 1);
			q_b.val[1] = veorq_u32(q_tmp.val[1], q_b.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 7);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 25);
			q_c.val[1] = vextq_u32(q_c.val[1], q_c.val[1], 1);
			q_c.val[1] = veorq_u32(q_tmp.val[1], q_c.val[1]);
			//6
			q_tmp.val[0] = vaddq_u32(q_a.val[1], q_a.val[0]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 9);
			q_tmp.val[2] = vaddq_u32(q_b.val[1], q_b.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 23);
			q_a.val[2] = veorq_u32(q_tmp.val[1], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 9);
			q_tmp.val[3] = vaddq_u32(q_c.val[1], q_c.val[0]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 23);
			q_b.val[2] = veorq_u32(q_tmp.val[1], q_b.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 9);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 23);
			q_c.val[2] = veorq_u32(q_tmp.val[1], q_c.val[2]);
			//7
			q_tmp.val[0] = vaddq_u32(q_a.val[2], q_a.val[1]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 13);
			q_tmp.val[2] = vaddq_u32(q_b.val[2], q_b.val[1]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 19);
			q_a.val[3] = veorq_u32(q_tmp.val[1], q_a.val[3]);
				q_a.val[1] = vextq_u32(q_a.val[1], q_a.val[1], 3);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 13);
			q_tmp.val[3] = vaddq_u32(q_c.val[2], q_c.val[1]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 19);
			q_b.val[3] = veorq_u32(q_tmp.val[1], q_b.val[3]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 13);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 19);
			q_c.val[3] = veorq_u32(q_tmp.val[1], q_c.val[3]);
			q_b.val[1] = vextq_u32(q_b.val[1], q_b.val[1], 3);
			q_c.val[1] = vextq_u32(q_c.val[1], q_c.val[1], 3);

			//8
			q_tmp.val[0] = vaddq_u32(q_a.val[3], q_a.val[2]);
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[0], 18);
			q_tmp.val[2] = vaddq_u32(q_b.val[3], q_b.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[0], 14);
			q_a.val[0] = veorq_u32(q_tmp.val[1], q_a.val[0]);
				ba_b.val[0] = vaddq_u32(q_a.val[0], ba_b.val[0]);
					one =	32 * (3 * (ba_b.val[0][0] & (N - 1)) + 0);
					__builtin_prefetch(&W[one + 0]);
					__builtin_prefetch(&W[one + 8]);
					__builtin_prefetch(&W[one + 16]);
					__builtin_prefetch(&W[one + 24]);
			
			q_a.val[2] = vextq_u32(q_a.val[2], q_a.val[2], 2);
			q_b.val[2] = vextq_u32(q_b.val[2], q_b.val[2], 2);
			
			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[2], 18);
			q_tmp.val[3] = vaddq_u32(q_c.val[3], q_c.val[2]);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[2], 14);
			q_c.val[2] = vextq_u32(q_c.val[2], q_c.val[2], 2);
			q_b.val[3] = vextq_u32(q_b.val[3], q_b.val[3], 1);
			q_b.val[0] = veorq_u32(q_tmp.val[1], q_b.val[0]);
				bb_b.val[0] = vaddq_u32(q_b.val[0], bb_b.val[0]);
					two =	32 * (3 * (bb_b.val[0][0] & (N - 1)) + 1);
					__builtin_prefetch(&W[two + 0]);
					__builtin_prefetch(&W[two + 8]);
					__builtin_prefetch(&W[two + 16]);
					__builtin_prefetch(&W[two + 24]);

			q_tmp.val[1] = vshlq_n_u32(q_tmp.val[3], 18);
			q_tmp.val[1] = vsriq_n_u32(q_tmp.val[1], q_tmp.val[3], 14);
			q_a.val[3] = vextq_u32(q_a.val[3], q_a.val[3], 1);
			q_c.val[3] = vextq_u32(q_c.val[3], q_c.val[3], 1);
			q_c.val[0] = veorq_u32(q_tmp.val[1], q_c.val[0]);
				bc_b.val[0] = vaddq_u32(q_c.val[0], bc_b.val[0]);
					three = 32 * (3 * (bc_b.val[0][0] & (N - 1)) + 2);
					__builtin_prefetch(&W[three + 0]);
					__builtin_prefetch(&W[three + 8]);
					__builtin_prefetch(&W[three + 16]);
					__builtin_prefetch(&W[three + 24]);
		}

		x.val[0] = vld1q_u32(&W[one +  0]);
		ba_b.val[1] = vaddq_u32(q_a.val[1], ba_b.val[1]);
		ba_b.val[2] = vaddq_u32(q_a.val[2], ba_b.val[2]);
		ba_b.val[3] = vaddq_u32(q_a.val[3], ba_b.val[3]);
		x.val[1] = vld1q_u32(&W[one +  4]);
		bb_b.val[1] = vaddq_u32(q_b.val[1], bb_b.val[1]);
		bb_b.val[2] = vaddq_u32(q_b.val[2], bb_b.val[2]);
		bb_b.val[3] = vaddq_u32(q_b.val[3], bb_b.val[3]);
		x.val[2] = vld1q_u32(&W[one +  8]);
		bc_b.val[1] = vaddq_u32(q_c.val[1], bc_b.val[1]);
		bc_b.val[2] = vaddq_u32(q_c.val[2], bc_b.val[2]);
		bc_b.val[3] = vaddq_u32(q_c.val[3], bc_b.val[3]);
		x.val[3] = vld1q_u32(&W[one + 12]);
	}

	vst1q_u32(&B[0],       ba_a.val[0]);
	vst1q_u32(&B[4],       ba_a.val[1]);
	vst1q_u32(&B[8],       ba_a.val[2]);
	vst1q_u32(&B[12],      ba_a.val[3]);
	vst1q_u32(&B[16 + 0],  ba_b.val[0]);
	vst1q_u32(&B[16 + 4],  ba_b.val[1]);
	vst1q_u32(&B[16 + 8],  ba_b.val[2]);
	vst1q_u32(&B[16 + 12], ba_b.val[3]);

	vst1q_u32(&B[32 + 0],  		bb_a.val[0]);
	vst1q_u32(&B[32 + 4],  		bb_a.val[1]);
	vst1q_u32(&B[32 + 8],  		bb_a.val[2]);
	vst1q_u32(&B[32 + 12], 		bb_a.val[3]);
	vst1q_u32(&B[32 + 16 + 0],  bb_b.val[0]);
	vst1q_u32(&B[32 + 16 + 4],  bb_b.val[1]);
	vst1q_u32(&B[32 + 16 + 8],  bb_b.val[2]);
	vst1q_u32(&B[32 + 16 + 12], bb_b.val[3]);

	vst1q_u32(&B[64 + 0],  		bc_a.val[0]);
	vst1q_u32(&B[64 + 4],  		bc_a.val[1]);
	vst1q_u32(&B[64 + 8],  		bc_a.val[2]);
	vst1q_u32(&B[64 + 12], 		bc_a.val[3]);
	vst1q_u32(&B[64 + 16 + 0],  bc_b.val[0]);
	vst1q_u32(&B[64 + 16 + 4],  bc_b.val[1]);
	vst1q_u32(&B[64 + 16 + 8],  bc_b.val[2]);
	vst1q_u32(&B[64 + 16 + 12], bc_b.val[3]);

	scrypt_shuffle(&B[0  + 0]);
	scrypt_shuffle(&B[16 + 0]);
	scrypt_shuffle(&B[0 + 32]);
	scrypt_shuffle(&B[16 + 32]);
	scrypt_shuffle(&B[0 + 64]);
	scrypt_shuffle(&B[16 + 64]);
}
#endif
