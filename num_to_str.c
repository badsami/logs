#include "num_to_str.h"
#include "isa.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//// Intrinsics
// Declare the specific intrinsics used below as extern, rather than including the 1025-line-long
// intrin.h header 
extern u8 _BitScanReverse(u32* msb_idx, u32 mask);

#if defined(ISA_x64)
extern u8 _BitScanReverse64(u32* msb_idx, u64 mask);
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Utilities
// x86 32-bit compilation target, without SSE
#if defined(ISA_x86) && defined(_M_IX86_FP) && (_M_IX86_FP == 0)
#  include "win32_x86_crt_no_sse.c"
#endif

static const schar8 hex_digits[] = "0123456789ABCDEF";

// This may look useless, but it will later be expanded to support platforms currently unsupported
static inline u32 bsr32(u32 value)
{
  u32 msb_idx;
  _BitScanReverse(&msb_idx, value);
  return msb_idx;
}

static inline u32 bsr64(u64 value)
{
#if defined(ISA_x64)
  u32 msb_idx;
  _BitScanReverse64(&msb_idx, value);
  return msb_idx;
#else
  // Assumming little endian order
  u32* halves            = (u32*)&value;
  u32  relevant_half_idx = halves[1] != 0;
  u32  relevant_half     = halves[relevant_half_idx];
  u32  msb_idx           = bsr32(relevant_half);
  u32  msb_idx_offset    = relevant_half_idx << 5;
  
  return msb_idx + msb_idx_offset;
#endif
}

u32 u32_str_size(u32 num)
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

u32 u64_str_size(u64 num)
{
  // bsr(num|0b1) = msb_idx, with:
  // - bsr: the Bit Scan Reverse instruction, num being ORed for num=0 to return 1
  // - msb_idx: index of the most significant bit of num, with:
  //   • 0 being the index of the lowest, rightmost bit
  //   • 63 being the index of the highest, leftmost bit
  //
  // msb_idx can be extrapolated to get the lowest and highest value num can take. For instance:
  // bsr(num|0b1) = 3 = msb_idx, meaning num is of the form 0b1xxx, x being either a 0 or 1 bit. 
  // The lowest num can be is 0b1000, while the highest it can be is 0b1111. Another way to write
  // the minimum and maximum value of num is:
  //
  //   bsr(num|0b1) = msb_idx
  //   msb_to_min_value(msb_idx) =  1 << msb_idx
  //   msb_to_max_value(msb_idx) = (2 << msb_idx) - 1
  //
  // So the maximum number of digit of num is the number of digit of (2 << msb_idx) - 1 in base 10.
  // Since num is a 64-bit value, msb_idx is always in the range [0;63], meaning the maximum value
  // (and by extension the maximum number of digits) num needs to be represented in base 10 can be
  // pre-computed:
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

  // For some values of msb_idx, the number of base 10 digits in the minimum and maximum values may
  // vary by one. That means the maximum value represented by msb_idx alone cannot be used to get
  // the number of base 10 digit in num. However, by checking whether num is less than the first
  // base 10 number with the same number of digits as the maximum value, the digit count can be
  // offset by -1 to get the final result:
  static const u64 thresholds[20] =
  {
    0llu,
    10llu,
    100llu,
    1000llu,
    10000llu,
    100000llu,
    1000000llu,
    10000000llu,
    100000000llu,
    1000000000llu,
    10000000000llu,
    100000000000llu,
    1000000000000llu,
    10000000000000llu,
    100000000000000llu,
    1000000000000000llu,
    10000000000000000llu,
    100000000000000000llu,
    1000000000000000000llu,
    10000000000000000000llu
  };

  u32 msb_idx = bsr64(num | 0b1);

  u32 digit_count = msb_to_max_digit_count[msb_idx];
  digit_count -= (num < thresholds[digit_count - 1u]);
  return digit_count;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Formatting
u32 u32_to_str(schar8* buffer, u32 num)
{
  u32     digit_count = u32_str_size(num);
  schar8* num_str     = buffer + digit_count;
  do
  {
    num_str--;

    u32    quotient    = num / 10;
    schar8 right_digit = (schar8)(num - quotient * 10);

    *num_str = '0' + right_digit;
    num    = quotient;
  }
  while (num != 0);

  return digit_count;
}

u32 u64_to_str(schar8* buffer, u64 num)
{
  u32     digit_count = u64_str_size(num);
  schar8* num_str     = buffer + digit_count;
  do
  {
    num_str--;

    u64    quotient    = num / 10;
    schar8 right_digit = (schar8)(num - quotient * 10);

    *num_str = '0' + right_digit;
    num    = quotient;
  }
  while (num != 0);

  return digit_count;
}

u32 s32_to_str(schar8* buffer, s32 num)
{
  // The negative sign is always written. It is overwritten if it is unnecessary
  buffer[0] = '-';
  
  s32 is_neg = num < 0;

  // Branchless (is_neg ? -num : num)
  s32 pos_num          = (num ^ -is_neg) + is_neg;
  u32 pos_num_str_size = u32_to_str(buffer + is_neg, pos_num);

  return pos_num_str_size + is_neg;
}

u32 s64_to_str(schar8* buffer, s64 num)
{
  // The negative sign is always written. It is overwritten if it is unnecessary
  buffer[0] = '-';
  
  s64 is_neg = num < 0;

  // Branchless (is_neg ? -num : num)
  s64 pos_num          = (num ^ -is_neg) + is_neg;
  u32 pos_num_str_size = u64_to_str(buffer + is_neg, pos_num);

  return pos_num_str_size + (u32)is_neg;
}

u32 f32_to_str(schar8* buffer, f32 num)
{
  // Write the integral part
  s32 num_int = (s32)num;

  // The negative sign is always written. It is overwritten if it is unnecessary
  buffer[0] = '-';
  
  s32 is_neg = num < 0.f;
  // NOTE: computing the absolute value of num instead of resorting to two bit tricks to
  // conditionnaly negate num_int and MAX_FRAC_DIGITS reduces reciprocal throughput and latency.
  // Keep the bit tricks.

  // Branchless (is_neg ? -num : num)
  s32     pos_num_int          = (num_int ^ -is_neg) + is_neg;
  u32     pos_num_int_str_size = u32_to_str(buffer + is_neg, pos_num_int);
  u32     num_int_str_size     = pos_num_int_str_size + (u32)is_neg;
  schar8* num_str              = buffer + num_int_str_size;

  // Add period separator
  *num_str++ = '.';

  // The fractional part may have leading zeros (e.g. 0.001), so it cannot just be multiplied by a
  // fixed amount then passed to u32_to_str() because the leading zeros will not be added (e.g 0.1)
  const u32 frac_max_digit  = 6;
  const u32 frac_multiplier = 1000000;
  
  s32     num_frac_multiplier = (frac_multiplier ^ -is_neg) + is_neg;
  u32     num_frac            = (u32)((num - num_int) * num_frac_multiplier);
  schar8* num_frac_str        = num_str + frac_max_digit;

  do
  {
    num_frac_str--;
    
    u32    quotient = num_frac / 10;
    schar8 digit    = (schar8)(num_frac - quotient * 10);
    *num_frac_str   = '0' + digit;

    num_frac = quotient;
  }
  while (num_frac != 0);

  // Fill remaining characters with 0s
  while (num_frac_str-- != num_str)
  {
    *num_frac_str = '0';
  }

  const u32 dot_separator_size = 1;
  return num_int_str_size + dot_separator_size + frac_max_digit;
}

u32 u32_to_sized_hex_str(schar8* buffer, u32 num, u32 nibble_count)
{
  schar8* end     = buffer + 1;
  schar8* num_str = end + nibble_count;
  
  do
  {
    u32 nibble_idx = num & 0xF;
    *num_str = hex_digits[nibble_idx];
    num_str--;
    num >>= 4;
  }
  while (num_str != end);

  buffer[0] = '0';
  buffer[1] = 'x';
   
  return nibble_count + 2;
}

u32 u64_to_sized_hex_str(schar8* buffer, u64 num, u32 nibble_count)
{
  schar8* end     = buffer + 1;
  schar8* num_str = end + nibble_count;
  
  do
  {
    u32 nibble_idx = num & 0xF;
    *num_str = hex_digits[nibble_idx];
    num_str--;
    num >>= 4;
  }
  while (num_str != end);

  buffer[0] = '0';
  buffer[1] = 'x';
   
  return nibble_count + 2;
}

u32 u32_to_sized_bin_str(schar8* buffer, u32 num, u32 bit_count)
{
  schar8* end     = buffer + 1;
  schar8* num_str = end + bit_count;
  
  do
  {
    schar8 bit = num & 0b1;
    *num_str = '0' + bit;
    num_str--;
    num >>= 1;
  }
  while (num_str != end);

  buffer[0] = '0';
  buffer[1] = 'b';
   
  return bit_count + 2;
}

u32 u64_to_sized_bin_str(schar8* buffer, u64 num, u32 bit_count)
{
  schar8* end     = buffer + 1;
  schar8* num_str = end + bit_count;
  
  do
  {
    schar8 bit = num & 0b1;
    *num_str = '0' + bit;
    num_str--;
    num >>= 1;
  }
  while (num_str != end);

  buffer[0] = '0';
  buffer[1] = 'b';
   
  return bit_count + 2;
}

u32 u32_to_min_hex_str(schar8* buffer, u32 num)
{
  // 2 prefix char (0x) + 1 char minimum (0) + msb_idx / 4 (for each additional nibble)
  u32     msb_idx  = bsr64(num | 0b1);
  u32     str_size = 3u + msb_idx / 4u;
  schar8* num_str  = buffer + str_size;
  
  do
  {
    num_str--;

    u32 digit_idx = num & 0xF;
    *num_str = hex_digits[digit_idx];
    num >>= 4;
  }
  while (num != 0u);

  *(num_str - 1) = 'x';
  *(num_str - 2) = '0';
  
  return str_size;
}

u32 u64_to_min_hex_str(schar8* buffer, u64 num)
{
  // 2 prefix char (0x) + 1 char minimum (0) + msb_idx / 4 (for each additional nibble)
  u32     msb_idx  = bsr64(num | 0b1);
  u32     str_size = 3u + msb_idx / 4u;
  schar8* num_str  = buffer + str_size;
  
  do
  {
    num_str--;

    u32 digit_idx = num & 0xF;
    *num_str = hex_digits[digit_idx];
    num >>= 4;
  }
  while (num != 0llu);

  *(num_str - 1) = 'x';
  *(num_str - 2) = '0';
  
  return str_size;
}

u32 u32_to_min_bin_str(schar8* buffer, u32 num)
{
  // 2 characters for the "0b" prefix + at least 1 bit = 3
  u32 str_size = bsr64(num | 0b1);
  str_size += 3u;
  schar8* num_str = buffer + str_size;
  
  do
  {
    num_str--;
    schar8 bit = num & 0b1;
    *num_str = '0' + bit;
    num >>= 1;
  } while (num != 0u);

  *(num_str - 1) = 'b';
  *(num_str - 2) = '0';
  
  return str_size;
}

u32 u64_to_min_bin_str(schar8* buffer, u64 num)
{
  // 2 characters for the "0b" prefix + at least 1 bit = 3
  u32 str_size = bsr64(num | 0b1);
  str_size += 3u;
  schar8* num_str = buffer + str_size;
  
  do
  {
    num_str--;
    schar8 bit = num & 0b1;
    *num_str = '0' + bit;
    num >>= 1;
  } while (num != 0llu);

  *(num_str - 1) = 'b';
  *(num_str - 2) = '0';
  
  return str_size;
}
