#include "to_str_utilities.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Intrinsics
// Declare the specific intrinsics used below as extern, rather than including the 1025-line-long
// intrin.h header 
extern u8 _BitScanReverse(u32* msb_idx, u32 mask);
extern u8 _BitScanReverse64(u32* msb_idx, u64 mask);


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Intrisics wrapper
u32 bsr32(u32 num)
{
  u32 msb_idx;
  _BitScanReverse(&msb_idx, num);
  return msb_idx;
}


u32 bsr64(u64 num)
{
  u32 msb_idx;
  _BitScanReverse64(&msb_idx, num);
  return msb_idx;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Numerals count in a number
u32 u32_digit_count(u32 num)
{
  // https://commaok.xyz/post/lookup_tables/
  static const u64 table[32] =
  {
    4294967296,                                         // (1  << 32)
    8589934582,  8589934582,  8589934582,               // (2  << 32) - 10
    12884901788, 12884901788, 12884901788,              // (3  << 32) - 100
    17179868184, 17179868184, 17179868184,              // (4  << 32) - 1000
    21474826480, 21474826480, 21474826480, 21474826480, // (5  << 32) - 10000
    25769703776, 25769703776, 25769703776,              // (6  << 32) - 100000
    30063771072, 30063771072, 30063771072,              // (7  << 32) - 1000000
    34349738368, 34349738368, 34349738368, 34349738368, // (8  << 32) - 10000000
    38554705664, 38554705664, 38554705664,              // (9  << 32) - 100000000
    41949672960, 41949672960, 41949672960,              // (10 << 32) - 1000000000
    42949672960, 42949672960                            // (10 << 32)
  };

  u32 msb_idx = bsr32(num | 0b1);
  return (num + table[msb_idx]) >> 32;
}


u32 u64_digit_count(u64 num)
{
  static const u32 msb_to_max_digit_count[64] =
  {
    1, 1, 1,
    2, 2, 2,
    3, 3, 3,
    4, 4, 4, 4,
    5, 5, 5,
    6, 6, 6,
    7, 7, 7, 7,
    8, 8, 8,
    9, 9, 9,
    10, 10, 10, 10,
    11, 11, 11,
    12, 12, 12,
    13, 13, 13, 13,
    14, 14, 14,
    15, 15, 15,
    16, 16, 16, 16,
    17, 17, 17,
    18, 18, 18,
    19, 19, 19, 19,
    20
  };
  
  static const u64 thresholds[20] =
  {
    0ULL,
    10ULL,
    100ULL,
    1000ULL,
    10000ULL,
    100000ULL,
    1000000ULL,
    10000000ULL,
    100000000ULL,
    1000000000ULL,
    10000000000ULL,
    100000000000ULL,
    1000000000000ULL,
    10000000000000ULL,
    100000000000000ULL,
    1000000000000000ULL,
    10000000000000000ULL,
    100000000000000000ULL,
    1000000000000000000ULL,
    10000000000000000000ULL
  };

  u32 msb_idx = bsr64(num | 0b1);

  u32 digit_count = msb_to_max_digit_count[msb_idx];
  digit_count -= (num < thresholds[digit_count - 1u]);
  return digit_count;
}


u32 u32_bit_count(u32 num)
{
  return bsr32(num | 0b1) + 1u;
}


u32 u64_bit_count(u64 num)
{
  return bsr64(num | 0b1) + 1u;
}


u32 u32_nibble_count(u32 num)
{
  u32 msb_idx      = bsr32(num | 0b1);
  u32 nibble_count = 1u + (msb_idx >> 2);

  return nibble_count;
}


u32 u64_nibble_count(u64 num)
{
  u32 msb_idx      = bsr64(num | 0b1);
  u32 nibble_count = 1u + (msb_idx >> 2);
  
  return nibble_count;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Floating-point specific values
u32 f32_is_a_number(f32 num)
{
  const u32 EXPONENT_ALL_ONE = 0x7F800000;
  
  u32 num_bits = *(u32*)&num;
  
  return (num_bits & EXPONENT_ALL_ONE) != EXPONENT_ALL_ONE;
}
