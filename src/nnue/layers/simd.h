/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2024 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef STOCKFISH_SIMD_H_INCLUDED
#define STOCKFISH_SIMD_H_INCLUDED

#if defined(USE_AVX2)
    #include <immintrin.h>

#elif defined(USE_SSE41)
    #include <smmintrin.h>

#elif defined(USE_SSSE3)
    #include <tmmintrin.h>

#elif defined(USE_SSE2)
    #include <emmintrin.h>

#elif defined(USE_NEON)
    #include <arm_neon.h>

#elif defined(USE_WASM_SIMD)
    #include <wasm_simd128.h>
#endif

namespace Stockfish::Simd {

#if defined(USE_AVX512) || defined(USE_AVX512F)

[[maybe_unused]] static int m512_hadd(__m512i sum, int bias) {
    return _mm512_reduce_add_epi32(sum) + bias;
}

/*
      Parameters:
        sum0 = [zmm0.i128[0], zmm0.i128[1], zmm0.i128[2], zmm0.i128[3]]
        sum1 = [zmm1.i128[0], zmm1.i128[1], zmm1.i128[2], zmm1.i128[3]]
        sum2 = [zmm2.i128[0], zmm2.i128[1], zmm2.i128[2], zmm2.i128[3]]
        sum3 = [zmm3.i128[0], zmm3.i128[1], zmm3.i128[2], zmm3.i128[3]]

      Returns:
        ret = [
          reduce_add_epi32(zmm0.i128[0]), reduce_add_epi32(zmm1.i128[0]), reduce_add_epi32(zmm2.i128[0]), reduce_add_epi32(zmm3.i128[0]),
          reduce_add_epi32(zmm0.i128[1]), reduce_add_epi32(zmm1.i128[1]), reduce_add_epi32(zmm2.i128[1]), reduce_add_epi32(zmm3.i128[1]),
          reduce_add_epi32(zmm0.i128[2]), reduce_add_epi32(zmm1.i128[2]), reduce_add_epi32(zmm2.i128[2]), reduce_add_epi32(zmm3.i128[2]),
          reduce_add_epi32(zmm0.i128[3]), reduce_add_epi32(zmm1.i128[3]), reduce_add_epi32(zmm2.i128[3]), reduce_add_epi32(zmm3.i128[3])
        ]
    */
[[maybe_unused]] static __m512i
m512_hadd128x16_interleave(__m512i sum0, __m512i sum1, __m512i sum2, __m512i sum3) {

    __m512i sum01a = _mm512_unpacklo_epi32(sum0, sum1);
    __m512i sum01b = _mm512_unpackhi_epi32(sum0, sum1);

    __m512i sum23a = _mm512_unpacklo_epi32(sum2, sum3);
    __m512i sum23b = _mm512_unpackhi_epi32(sum2, sum3);

    __m512i sum01 = _mm512_add_epi32(sum01a, sum01b);
    __m512i sum23 = _mm512_add_epi32(sum23a, sum23b);

    __m512i sum0123a = _mm512_unpacklo_epi64(sum01, sum23);
    __m512i sum0123b = _mm512_unpackhi_epi64(sum01, sum23);

    return _mm512_add_epi32(sum0123a, sum0123b);
}

[[maybe_unused]] static void m512_add_dpbusd_epi32(__m512i& acc, __m512i a, __m512i b) {

    #if defined(USE_VNNI)
    acc = _mm512_dpbusd_epi32(acc, a, b);
    #elif defined(USE_AVX512F)
    acc = _mm512_add_epi32(
      acc, __builtin_shufflevector(
             _mm256_madd_epi16(_mm256_maddubs_epi16(__builtin_shufflevector(a, a, 0, 1, 2, 3),
                                                    __builtin_shufflevector(b, b, 0, 1, 2, 3)),
                               _mm256_set1_epi16(1)),
             _mm256_madd_epi16(_mm256_maddubs_epi16(__builtin_shufflevector(a, a, 4, 5, 6, 7),
                                                    __builtin_shufflevector(b, b, 4, 5, 6, 7)),
                               _mm256_set1_epi16(1)),
             0, 1, 2, 3, 4, 5, 6, 7));
    #else
    __m512i product0 = _mm512_maddubs_epi16(a, b);
    product0         = _mm512_madd_epi16(product0, _mm512_set1_epi16(1));
    acc              = _mm512_add_epi32(acc, product0);
    #endif
}

#endif

#if defined(USE_AVX2)

[[maybe_unused]] static int m256_hadd(__m256i sum, int bias) {
    __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
    sum128         = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_BADC));
    sum128         = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_CDAB));
    return _mm_cvtsi128_si32(sum128) + bias;
}

[[maybe_unused]] static void m256_add_dpbusd_epi32(__m256i& acc, __m256i a, __m256i b) {

    #if defined(USE_VNNI)
    acc = _mm256_dpbusd_epi32(acc, a, b);
    #else
    __m256i product0 = _mm256_maddubs_epi16(a, b);
    product0         = _mm256_madd_epi16(product0, _mm256_set1_epi16(1));
    acc              = _mm256_add_epi32(acc, product0);
    #endif
}

#endif

#if defined(USE_SSSE3)

[[maybe_unused]] static int m128_hadd(__m128i sum, int bias) {
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4E));  //_MM_PERM_BADC
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0xB1));  //_MM_PERM_CDAB
    return _mm_cvtsi128_si32(sum) + bias;
}

[[maybe_unused]] static void m128_add_dpbusd_epi32(__m128i& acc, __m128i a, __m128i b) {

    __m128i product0 = _mm_maddubs_epi16(a, b);
    product0         = _mm_madd_epi16(product0, _mm_set1_epi16(1));
    acc              = _mm_add_epi32(acc, product0);
}

#endif

#if defined(USE_NEON_DOTPROD)

[[maybe_unused]] static void
dotprod_m128_add_dpbusd_epi32(int32x4_t& acc, int8x16_t a, int8x16_t b) {

    acc = vdotq_s32(acc, a, b);
}
#endif

#if defined(USE_NEON)

[[maybe_unused]] static int neon_m128_reduce_add_epi32(int32x4_t s) {
    #if USE_NEON >= 8
    return vaddvq_s32(s);
    #else
    return s[0] + s[1] + s[2] + s[3];
    #endif
}

[[maybe_unused]] static int neon_m128_hadd(int32x4_t sum, int bias) {
    return neon_m128_reduce_add_epi32(sum) + bias;
}

#endif

#if USE_NEON >= 8
[[maybe_unused]] static void neon_m128_add_dpbusd_epi32(int32x4_t& acc, int8x16_t a, int8x16_t b) {

    int16x8_t product0 = vmull_s8(vget_low_s8(a), vget_low_s8(b));
    int16x8_t product1 = vmull_high_s8(a, b);
    int16x8_t sum      = vpaddq_s16(product0, product1);
    acc                = vpadalq_s16(acc, sum);
}
#endif

#if defined(USE_WASM_SIMD)

[[maybe_unused]] static int wasm_i32x4_reduce_add(__i32x4 s) {
    return wasm_i32x4_extract_lane(s, 0) + wasm_i32x4_extract_lane(s, 1)
         + wasm_i32x4_extract_lane(s, 2) + wasm_i32x4_extract_lane(s, 3);
}

[[maybe_unused]] static int wasm_i32x4_hadd(__i32x4 sum, int bias) {
    return wasm_i32x4_reduce_add(sum) + bias;
}

[[maybe_unused]] static void wasm_i32x4_add_dpbusd_epi32(__i32x4& acc, __i8x16 a, __i8x16 b) {
#if defined(__wasm_relaxed_simd__) && defined(USE_WASM_SIMD_RELAXED)
    acc = __builtin_wasm_dot_i8x16_i7x16_add_s_i32x4(b, a, acc);
#else
    a = wasm_i8x16_shuffle(a, a, 0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15);
    b = wasm_i8x16_shuffle(b, b, 0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15);
    __i16x8 a_lo     = wasm_i16x8_extend_low_i8x16(a);
    __i16x8 a_hi     = wasm_i16x8_extend_high_i8x16(a);
    __i16x8 b_lo     = wasm_i16x8_extend_low_i8x16(b);
    __i16x8 b_hi     = wasm_i16x8_extend_high_i8x16(b);
    __i32x4 product0 = wasm_i32x4_dot_i16x8(a_lo, b_lo);
    __i32x4 product1 = wasm_i32x4_dot_i16x8(a_hi, b_hi);
    acc              = wasm_i32x4_add(acc, wasm_i32x4_add(product0, product1));
#endif
}

#endif
}

#endif  // STOCKFISH_SIMD_H_INCLUDED
