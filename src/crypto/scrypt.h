#ifndef SCRYPT_H
#define SCRYPT_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include "util.h"
#include "uint256.h"
#include "utilstrencodings.h"


static const int SCRYPT_SCRATCHPAD_SIZE = 134218239;
static const int N = 1048576;

int scrypt_best_throughput();

bool scrypt_N_1_1_256_multi(void *input, uint256 hashTarget, int *nHashesDone, unsigned char *scratchbuf);

void scryptHash(const void *input, char *output);
extern unsigned char *scrypt_buffer_alloc();
extern "C" void scrypt_core(uint32_t *X, uint32_t *V, int N);
extern "C" void sha256_transform(uint32_t *state, const uint32_t *block, int swap);

#if defined(__x86_64__) && !defined(USE_AVX2)
#define SCRYPT_MAX_WAYS 12
#define HAVE_SCRYPT_3WAY 1
#define HAVE_SHA256_4WAY 1
#define scrypt_best_throughput() 3;
extern "C" int sha256_use_4way();
extern "C" void sha256_init_4way(uint32_t *state);
extern "C" void sha256_transform_4way(uint32_t *state, const uint32_t *block, int swap);
extern "C" void scrypt_core_3way(uint32_t *X, uint32_t *V, int N);
#endif

#if defined(__x86_64__) && defined(USE_AVX2)
#define SCRYPT_MAX_WAYS 24
#define HAVE_SCRYPT_6WAY 1
#define HAVE_SHA256_4WAY 1
#define HAVE_SHA256_8WAY 1
#define scrypt_best_throughput() 6;
extern "C" int sha256_use_8way();
extern "C" void sha256_init_8way(uint32_t *state);
extern "C" void sha256_transform_8way(uint32_t *state, const uint32_t *block, int swap);
extern "C" int sha256_use_4way();
extern "C" void sha256_init_4way(uint32_t *state);
extern "C" void sha256_transform_4way(uint32_t *state, const uint32_t *block, int swap);
extern "C" void scrypt_core_6way(uint32_t *X, uint32_t *V, int N);
#endif

#if defined(__i386__)
#define SCRYPT_MAX_WAYS 4
#define HAVE_SHA256_4WAY 1
#define scrypt_best_throughput() 1
extern "C" void scrypt_core(uint32_t *X, uint32_t *V, int N);
extern "C" int sha256_use_4way();
extern "C" void sha256_init_4way(uint32_t *state);
extern "C" void sha256_transform_4way(uint32_t *state, const uint32_t *block, int swap);
#endif

static inline uint32_t swab32(uint32_t v)
{
    return bswap_32(v);
}
#endif
