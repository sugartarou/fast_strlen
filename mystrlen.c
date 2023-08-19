/**
 * @file mystrlen.c
 * @author sugartarou
 * @brief strlenの高速化実験
 * 引用元：https://qiita.com/i_saint/items/8842262da9a981c8e9e8
 * @version 0.1
 * @date 2023-08-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <x86intrin.h>

#define N (2147483648ULL)
#define ANS (N - 1)

#define assert(n) if(n != ANS) { printf("Error! %ld != %lld\n", n, ANS); }
#define result(msg, st, et) printf("%s \t%.2lfms\n", msg, (et - st))

void now_msec(double *t) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	*t = tv.tv_sec * 1e3 + tv.tv_usec * 1e-3;
}

size_t strlen_simple(const char *str) {
	const char* p = str;
	while(*p) p++;
	return (size_t)(p - str);
}

#define HI (0x8080808080808080)
#define LO (0x0101010101010101)

int countr_zero(uint64_t v) {
	return _tzcnt_u64(v);
}

size_t strlen_u64(const char* str) {
	// 8 byte align ではない先頭部分を処理
    const char* aligned = (const char*)(((size_t)(str) + 0x7) & ~0x7);
    for (const char* p = str; p != aligned; ++p) {
        if (*p == 0) {
            return (size_t)(p - str);
        }
    }

	// 8 byte 単位でまとめて処理
	// (packed - LO) & ~packed & HI をかけると
	// 元が0(ヌル文字)のbyteは0x80, そうでないbyteは0x00になる
	// これでヌル文字が入ってるかどうかの判定を8byteまとめて処理できる
	// さらにcountr_zeroで右から0が何bit連続しているかをカウントし、
	// それを8で割る(右に3bitシフト)と最初のヌル文字のインデックスが算出できる
	for (const uint64_t* qp = (const uint64_t*)aligned; ; ++qp) {
        uint64_t packed = *qp;
        uint64_t mask = (packed - LO) & ~packed & HI;
        if (mask != 0) {
            int idx = countr_zero(mask) >> 3;
            return (size_t)(qp) - (size_t)(str) + idx;
        }
    }
}

size_t strlen_sse(const char* str) {
	const __m128i zero = _mm_set1_epi8(0);
	const __m128i* sxp = (const __m128i*)((size_t)str & ~0xf);

	// 16 byte align ではない先頭部分を処理
	size_t mod = (size_t)str & 0xf;
	if (mod) {
		__m128i sx 	 = _mm_load_si128(sxp);
		__m128i mask = _mm_cmpeq_epi8(zero, sx);
		if (!_mm_test_all_zeros(mask, mask)) {
			size_t remain = 16 - mod;
			for (size_t i = 0; i < remain; i++) {
				if (str[i] == 0) {
					return i;
				}
			}
		}
		++sxp;
	}

	// 16 byte 単位で処理
	while(1) {
		__m128i sx   = _mm_load_si128(sxp);
		__m128i mask = _mm_cmpeq_epi8(zero, sx);
		if (!_mm_test_all_zeros(mask, mask)) {
			int idx = _mm_cmpistri(sx, sx, _SIDD_CMP_EQUAL_EACH | _SIDD_MASKED_NEGATIVE_POLARITY);
			return (size_t)sxp + idx - (size_t)str;
		}
		++sxp;
	}
}

int main() {
	char *str;
	double t1, t2, t3, t4, t5;
	size_t r1, r2, r3, r4;

	str = (char *)malloc(sizeof(char) * N);
	memset(str, 'a', N - 1);
	str[N - 1] = '\0';

	now_msec(&t1);
	r1 = strlen(str);
	now_msec(&t2);
	r2 = strlen_simple(str);
	now_msec(&t3);
	r3 = strlen_u64(str);
	now_msec(&t4);
	r4 = strlen_sse(str);
	now_msec(&t5);
	assert(r1);
	assert(r2);
	assert(r3);
	assert(r4);
	result("strlen\t", t1, t2);
	result("strlen_simple", t2, t3);
	result("strlen_u64", t3, t4);
	result("strlen_sse", t4, t5);
}
