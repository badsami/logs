#if defined(LOGS_ENABLED) && (LOGS_ENABLED != 0)
#include "logs.h"
#include "to_str_utilities.h"
#include "types_max_str_size.h"

#define _WIN32_WINNT 0x0501 // ATTACH_PARENT_PROCESS
#include <Windows.h>


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Private helpers
static void logs_close_output(logs_output_idx output_idx)
{
  u32 output_mask          = (1 << output_idx);
  u32 output_was_enabled   = logs.outputs_state_bits & output_mask;
  logs.outputs_state_bits &= ~output_mask;
  
  if (logs.buffer_end_idx != 0 && output_was_enabled)
  {
    // Flush buffered content to the output before closing it
    WriteFile(logs.outputs[output_idx], logs.buffer, logs.buffer_end_idx, 0, 0);
    
    // If there are no other enabled outputs, the content of the log buffer is no longer needed
    u32 any_output_open = (logs.outputs_state_bits != 0);
    logs.buffer_end_idx = any_output_open * logs.buffer_end_idx;
  }
  
  CloseHandle(logs.outputs[output_idx]);
  logs.outputs[output_idx] = 0;
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Global
struct logs logs =
{
  .buffer             = {0},
  .outputs            = {0, 0},
  .buffer_end_idx     = 0,
  .outputs_state_bits = 0
};




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Output management
// Console output
void logs_open_console_output(void)
{
  if (logs.outputs[LOGS_CONSOLE_OUTPUT] == 0)
  {
    // If this process already has a console, this will not allocate a new console
    s32 success = AttachConsole(ATTACH_PARENT_PROCESS);
    if (success == 0 && GetLastError() == ERROR_INVALID_HANDLE)
    {
      // This process doesn't have a console. Create a new one
      AllocConsole();
      SetConsoleTitle("Logs");

      logs.console_original_output_code_page = 0u;
    }
    else
    {
      logs.console_original_output_code_page = GetConsoleOutputCP();
    }

    SetConsoleOutputCP(CP_UTF8);

    const u32 SHARE_MODE = FILE_SHARE_READ | FILE_SHARE_WRITE;
    logs.outputs[LOGS_CONSOLE_OUTPUT] = CreateFileA("\\\\?\\CONOUT$", // lpFileName
                                                    GENERIC_WRITE,    // dwDesiredAccess
                                                    SHARE_MODE,       // dwShareMode
                                                    0,                // lpSecurityAttributes
                                                    OPEN_EXISTING,    // dwCreationDisposition
                                                    0,                // dwFlagsAndAttributes
                                                    0);               // hTemplateFile
    logs_enable_output(LOGS_CONSOLE_OUTPUT);
  }
}


// NOTE (Sami): this breaks Windows Terminal (the default Windows 11 console)
void logs_close_console_output(void)
{
  if (logs.outputs[LOGS_CONSOLE_OUTPUT] != 0)
  {
    logs_close_output(LOGS_CONSOLE_OUTPUT);

    if (logs.console_original_output_code_page != 0u)
    {
      SetConsoleOutputCP(logs.console_original_output_code_page);
    }
    
    // Free the console of this process
    FreeConsole();
  }
}


// File output
void logs_open_file_output_ascii(const char* file_path)
{
  if (logs.outputs[LOGS_FILE_OUTPUT] == 0)
  {
    const u32 SHARE_MODE = FILE_SHARE_READ | FILE_SHARE_WRITE;
    logs.outputs[LOGS_FILE_OUTPUT] = CreateFileA(file_path,        // lpFileName
                                                FILE_APPEND_DATA, // dwDesiredAccess
                                                SHARE_MODE,       // dwShareMode
                                                0,                // lpSecurityAttributes
                                                OPEN_ALWAYS,      // dwCreationDisposition
                                                0,                // dwFlagsAndAttributes
                                                0);               // hTemplateFile
    logs_enable_output(LOGS_FILE_OUTPUT);
  }
}


void logs_open_file_output_utf16(const WCHAR* file_path)
{
  if (logs.outputs[LOGS_FILE_OUTPUT] == 0)
  {
    const u32 SHARE_MODE = FILE_SHARE_READ | FILE_SHARE_WRITE;
    logs.outputs[LOGS_FILE_OUTPUT] = CreateFileW(file_path,        // lpFileName
                                                 FILE_APPEND_DATA, // dwDesiredAccess
                                                 SHARE_MODE,       // dwShareMode
                                                 0,                // lpSecurityAttributes
                                                 OPEN_ALWAYS,      // dwCreationDisposition
                                                 0,                // dwFlagsAndAttributes
                                                 0);               // hTemplateFile
    logs_enable_output(LOGS_FILE_OUTPUT);
  }
}


void logs_close_file_output(void)
{
  if (logs.outputs[LOGS_FILE_OUTPUT] != 0)
  {
    logs_close_output(LOGS_FILE_OUTPUT);
  }
}


// All outputs
void logs_disable_output(logs_output_idx output_idx)
{
  logs.outputs_state_bits &= ~(1 << output_idx);
}


void logs_enable_output(logs_output_idx output_idx)
{
  u32 is_open = (logs.outputs[output_idx] != 0);
  logs.outputs_state_bits |= (is_open << output_idx);
}


void logs_write(void)
{
  // Trust that the caller knows the log buffer is not empty
  // If an output is enabled, it is open. Parse the output state bits to select destination outputs
  u32 outputs_mask = logs.outputs_state_bits;
  u32 idx          = 0;
  while (outputs_mask != 0)
  {
    if (outputs_mask & 0b1)
    {
      WriteFile(logs.outputs[idx], logs.buffer, logs.buffer_end_idx, 0, 0);
    }
    
    idx++;
    outputs_mask >>= 1;
  }

  logs.buffer_end_idx = 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Memory
u32 logs_buffer_remaining_bytes()
{
  s32 difference      = LOGS_BUFFER_SIZE - logs.buffer_end_idx;
  u32 remaining_bytes = (difference > 0) ? difference : 0;

  return remaining_bytes;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Characters & strings logging
void log_utf8_character(char character)
{
  logs.buffer[logs.buffer_end_idx] = character;
  logs.buffer_end_idx += 1u;
}


void log_utf16_character(WCHAR character)
{
  log_utf16_str(&character, 1u);
}


// Append a chain of UTF-8-encoded characters (u8"Fluß") to the log buffer
void log_utf8_str(const char* str, u32 char_count)
{
  char* dest = (char*)(logs.buffer + logs.buffer_end_idx);
  
  const char* str_end = str + char_count;
  while (str < str_end)
  {
    *dest = *str;
    dest += 1;
    str  += 1;
  }
  
  logs.buffer_end_idx += char_count;
}


// Append a chain of UTF-16-encoded characters (u"çéÖ", L"çéÖ") to the log buffer
void log_utf16_str(const WCHAR* str, u32 wchar_count)
{
  // None of the functions appending content to the logs buffer make sure there is enough space to
  // write to. Lie about the available space in the logs buffer
  const s32 AVAILABLE_BYTES = ~(1 << 31);

  // TODO: implement our own WideCharToMultiByte()
  char* dest = (char*)(logs.buffer + logs.buffer_end_idx);
  s32 bytes_written = WideCharToMultiByte(CP_UTF8,         // CodePage,
                                          0,               // dwFlags,
                                          str,             // lpWideCharStr
                                          wchar_count,     // cchWideChar 
                                          dest,            // lpMultiByteStr
                                          AVAILABLE_BYTES, // cbMultiByte
                                          0,               // lpDefaultChar
                                          0);              // lpUsedDefaultChar

  logs.buffer_end_idx += bytes_written;
}


void log_utf8_ntstr(const char* str)
{
  char* dest      = (char*)(logs.buffer + logs.buffer_end_idx);
  char  character = *str;
  while (character != '\0')
  {
    *dest = character;
    dest += 1;
    str  += 1;
    character = *str;
  }
}

// Append a null-terminated ("nt") chain of UTF-16-encoded characters (u"çéÖ", L"çéÖ") to the
// log buffer
void log_utf16_ntstr(const WCHAR* str)
{
  // TODO: vVery much suboptimal 
  u32 wchar_count = 0u;
  while (str[wchar_count] != L'\0')
  {
    wchar_count += 1;
  }

  log_utf16_str(str, wchar_count);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Binary number logging
void log_sized_bin_s8 (s8  num, u32 bit_to_write_count) { log_sized_bin_u64((u64)num,      bit_to_write_count); }
void log_sized_bin_s16(s16 num, u32 bit_to_write_count) { log_sized_bin_u64((u64)num,      bit_to_write_count); }
void log_sized_bin_s32(s32 num, u32 bit_to_write_count) { log_sized_bin_u64((u64)num,      bit_to_write_count); }
void log_sized_bin_s64(s64 num, u32 bit_to_write_count) { log_sized_bin_u64((u64)num,      bit_to_write_count); }
void log_sized_bin_u8 (u8  num, u32 bit_to_write_count) { log_sized_bin_u64(num,           bit_to_write_count); }
void log_sized_bin_u16(u16 num, u32 bit_to_write_count) { log_sized_bin_u64(num,           bit_to_write_count); }
void log_sized_bin_u32(u32 num, u32 bit_to_write_count) { log_sized_bin_u64(num,           bit_to_write_count); }


void log_sized_bin_u64(u64 num, u32 bit_to_write_count)
{
  u8* const num_str_start = logs.buffer + logs.buffer_end_idx;
  u8*       dest          = num_str_start + bit_to_write_count;
  while (dest > num_str_start)
  {
    u8 bit = num & 0b1;
    
    dest -= 1;
    *dest = '0' + bit;
    
    num >>= 1;
  }

  logs.buffer_end_idx += bit_to_write_count;
}


void log_sized_bin_f32(f32 num, u32 bit_to_write_count) { log_sized_bin_u64(*(u32*)(&num), bit_to_write_count); }


void log_bin_s8 (s8  num) { log_sized_bin_u64((u64)num,      u32_bit_count((u32)num));    }
void log_bin_s16(s16 num) { log_sized_bin_u64((u64)num,      u32_bit_count((u32)num));    }
void log_bin_s32(s32 num) { log_sized_bin_u64((u64)num,      u32_bit_count((u32)num));    }
void log_bin_s64(s64 num) { log_sized_bin_u64((u64)num,      u64_bit_count((u64)num));    }
void log_bin_u8 (u8  num) { log_sized_bin_u64(num,           u32_bit_count(num));         }
void log_bin_u16(u16 num) { log_sized_bin_u64(num,           u32_bit_count(num));         }
void log_bin_u32(u32 num) { log_sized_bin_u64(num,           u32_bit_count(num));         }
void log_bin_u64(u64 num) { log_sized_bin_u64(num,           u64_bit_count(num));         }
void log_bin_f32(f32 num) { log_sized_bin_u64(*(u32*)(&num), u32_bit_count(*(u32*)&num)); }


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Decimal number logging
void log_sized_dec_s8 (s8  num, u32 digit_to_write_count) { log_sized_dec_u64((u64)num, digit_to_write_count); }
void log_sized_dec_s16(s16 num, u32 digit_to_write_count) { log_sized_dec_u64((u64)num, digit_to_write_count); }
void log_sized_dec_s32(s32 num, u32 digit_to_write_count) { log_sized_dec_u64((u64)num, digit_to_write_count); }


void log_sized_dec_s64(s64 num, u32 digit_to_write_count)
{
  s64 is_neg  = num < 0LLu;
  u64 pos_num = (num ^ -is_neg) + is_neg;

  logs.buffer[logs.buffer_end_idx] = '-'; // overwritten if unnecessary
  logs.buffer_end_idx += (u32)(is_neg);
  
  log_sized_dec_u64(pos_num, digit_to_write_count);
}


void log_sized_dec_u8 (u8  num, u32 digit_to_write_count) { log_sized_dec_u64(num, digit_to_write_count); }
void log_sized_dec_u16(u16 num, u32 digit_to_write_count) { log_sized_dec_u64(num, digit_to_write_count); }
void log_sized_dec_u32(u32 num, u32 digit_to_write_count) { log_sized_dec_u64(num, digit_to_write_count); }


void log_sized_dec_u64(u64 num, u32 digit_to_write_count)
{
  u8* const num_str_start = logs.buffer + logs.buffer_end_idx;
  u8*       dest          = num_str_start + digit_to_write_count;
  while (dest > num_str_start)
  {
    u64 quotient = num / 10LLu;
    u8  digit    = (u8)(num - (quotient * 10LLu));

    dest -= 1;
    *dest = '0' + digit;
    
    num = quotient;
  }
  
  logs.buffer_end_idx += digit_to_write_count;
}


static const f32 f32_frac_size_to_mul[F32_DEC_FRAC_MAX_STR_SIZE + 1] =
{
  1.f,
  10.f,
  100.f,
  1000.f,
  10000.f,
  100000.f,
  1000000.f,
  10000000.f,
  100000000.f,
  1000000000.f
};

void log_sized_dec_f32_number(f32 num, u32 frac_digit_to_write_count)
{
  u32 is_neg = num < 0.f;
  
  logs.buffer[logs.buffer_end_idx] = '-'; // overwritten if unnecessary
  logs.buffer_end_idx += is_neg;

  // Absolute values equal or greater to 8 388 608 are likely better represented as a s32 or s64
  // than as a 32-bit floating-point value (thereafter referred to as "f32"), for two reasons:
  // 
  // - Range of values: f32 values can be as large as +/- 3.4 x 10^38. I believe such large values
  // are never needed in video games. If they are, s32 can represent values up to +/- 2.15 x 10^9
  // and s64 can represent values up to +/- 9.22 x 10^18 instead, which I think would cover any
  // game programmer's needs
  // 
  // - Precision: starting at 8 388 608 and beyond, f32 values cannot have a fractional part.
  // Starting at 16 777 216 and beyond, adding 1 to a f32 value doesn't change it. The fractional
  // part is not very significant at this magnitude, but s32 and s64 retain a precision of 1 on the
  // full range of values they cover
  //
  // Nonetheless, 8 388 608 needs 23 bits to be represented, which would require a u32. So the full
  // range of the u32 type (+/- [0; 4 294 967 296]) may as well be supported, adding an extra
  // margin
  num = is_neg ? -num : num;
  if (num < 4294967296.f)
  {
    u32 num_int = (u32)num;
    log_dec_u32(num_int);
    
    if (num < 8388608.f)
    {
      // The same way values as large as +/- 3.4 x 10^38 are, in my view, never needed, I believe 
      // precision under 0.000001 is also never needed. 0.000001 is precise enough:
      // - if 1.0 = 1 meter,  0.000001 = 1 micrometer
      // - if 1.0 = 1 radian, 0.000001 = 1 / 6 283 185th of a circle = 0.000057 degrees
      f32 num_rounded = (f32)num_int;
      f32 num_frac    = num - num_rounded;
      if (num_frac >= 0.000001f)
      {
        log_character('.');

        // Fun fact: for floating-point values with exponent n (n < 23), the maximum count of decimal
        // fractional digit is 23 - n. Using the unbiased exponent u, this is the same as 150 - u.
        //
        //   u32 num_bits             = *(u32*)&num;
        //   s8  unbiased_exp         = (s8)((num_bits & 0x7F800000) >> 23);
        //   u8  max_frac_digit_count = 150u - unbiased_exp;

        // There can never be more fractional digits than the configured maximum allowed.
        // F32_DEC_FRAC_MAX_STR_SIZE is defined in types_max_str_size.h
        u32 num_frac_digit_to_write_count = (frac_digit_to_write_count < F32_DEC_FRAC_MAX_STR_SIZE) ?
                                       frac_digit_to_write_count : F32_DEC_FRAC_MAX_STR_SIZE;
        f32 num_frac_ext = num_frac * f32_frac_size_to_mul[num_frac_digit_to_write_count];
        u32 num_frac_int = (u32)num_frac_ext;

        log_sized_dec_u32(num_frac_int, num_frac_digit_to_write_count);
      }
    }
  }
  else
  {
    u8* dest = logs.buffer + logs.buffer_end_idx;
    dest[0] = 'b';
    dest[1] = 'i';
    dest[2] = 'g';

    logs.buffer_end_idx += 3u;
  }
}


void log_sized_dec_f32(f32 num, u32 frac_size)
{
  u32 is_a_number = f32_is_a_number(num);
  if (is_a_number)
  {
    log_sized_dec_f32_number(num, frac_size);
  }
  else
  {
    log_dec_f32_nan_or_inf(num);
  }
}

// The number of digits required to represent a signed integer and its unsigned cast are equal
void log_dec_s8 (s8  num) { log_sized_dec_s64(num, u32_digit_count((u32)num)); }
void log_dec_s16(s16 num) { log_sized_dec_s64(num, u32_digit_count((u32)num)); }
void log_dec_s32(s32 num) { log_sized_dec_s64(num, u32_digit_count((u32)num)); }
void log_dec_s64(s64 num) { log_sized_dec_s64(num, u64_digit_count((u64)num)); }
void log_dec_u8 (u8  num) { log_sized_dec_u64(num, u32_digit_count(num));      }
void log_dec_u16(u16 num) { log_sized_dec_u64(num, u32_digit_count(num));      }
void log_dec_u32(u32 num) { log_sized_dec_u64(num, u32_digit_count(num));      }
void log_dec_u64(u64 num) { log_sized_dec_u64(num, u64_digit_count(num));      }



void log_dec_f32_nan_or_inf(f32 num)
{
  u32 num_bits = *(u32*)&num;
  
  // num is +infinity, -infinity, qnan, -qnan, snan or -snan
  // The negative sign is overwritten if it is unnecessary
  u8* const num_str_start = logs.buffer + logs.buffer_end_idx;
  num_str_start[0] = '-'; // overwritten if unnecessary

  u8* dest = num_str_start;
  dest += num_bits >> 31;
 
  const u32 MANTISSA_MASK = 0x007FFFFF;
  u32 mantissa_bits = num_bits & MANTISSA_MASK;
  if (mantissa_bits == 0)
  {
    dest[0] = 'i';
    dest[1] = 'n';
    dest[2] = 'f';
    dest += 3;
  }
  else
  {
    // A nan is a float with all the exponent bits set and at least one fraction bit set.
    // A quiet nan (qnan) is a nan with the leftmost, highest fraction bit set.
    // A signaling nan (qnan) is a nan with the leftmost, highest fraction bit clear.
    // 's' - 'q' is 2, which is a multiple of 2. We can use this to change 's' to a 'q' by
    // offsetting 's' to 'q' without ifs
    u8 is_quiet_offset = (u8)((num_bits & 0x00400000) >> 21);
    
    dest[0] = 's' - is_quiet_offset;
    dest[1] = 'n';
    dest[2] = 'a';
    dest[3] = 'n';
    dest += 4;
  }

  logs.buffer_end_idx += (u32)(dest - num_str_start);
}


void log_dec_f32_number(f32 num)
{
  u32 is_neg = num < 0.f;
  
  logs.buffer[logs.buffer_end_idx] = '-'; // overwritten if unnecessary
  logs.buffer_end_idx += is_neg;

  // Absolute values equal or greater to 8 388 608 are likely better represented as a s32 or s64
  // than as a 32-bit floating-point value (thereafter referred to as "f32"), for two reasons:
  // 
  // - Range of values: f32 values can be as large as +/- 3.4 x 10^38. I believe such large values
  // are never needed in video games. If they are, s32 can represent values up to +/- 2.15 x 10^9
  // and s64 can represent values up to +/- 9.22 x 10^18 instead, which I think would cover any
  // game programmer's needs
  // 
  // - Precision: starting at 8 388 608 and beyond, f32 values cannot have a fractional part.
  // Starting at 16 777 216 and beyond, adding 1 to a f32 value doesn't change it. The fractional
  // part is not very significant at this magnitude, but s32 and s64 retain a precision of 1 on the
  // full range of values they cover
  //
  // Nonetheless, 8 388 608 needs 23 bits to be represented, which would require a u32. So the full
  // range of the u32 type (+/- [0; 4 294 967 296]) may as well be supported, adding an extra
  // margin
  num = is_neg ? -num : num;
  if (num < 4294967296.f)
  {
    u32 num_int = (u32)num;
    log_dec_u32(num_int);
    
    if (num < 8388608.f)
    {
      // The same way values as large as +/- 3.4 x 10^38 are, in my view, never needed, I believe 
      // precision under 0.000001 is also never needed. 0.000001 is precise enough:
      // - if 1.0 = 1 meter,  0.000001 = 1 micrometer
      // - if 1.0 = 1 radian, 0.000001 = 1 / 6 283 185th of a circle = 0.000057 degrees
      f32 num_rounded = (f32)num_int;
      f32 num_frac    = num - num_rounded;
      if (num_frac >= 0.000001f)
      {
        log_character('.');

        // Fun fact: for floating-point values with exponent n (n < 23), the maximum count of decimal
        // fractional digit is 23 - n. Using the unbiased exponent u, this is the same as 150 - u.
        //
        //   u32 num_bits             = *(u32*)&num;
        //   s8  unbiased_exp         = (s8)((num_bits & 0x7F800000) >> 23);
        //   u8  max_frac_digit_count = 150u - unbiased_exp;

        // F32_DEC_FRAC_MULT and F32_DEC_FRAC_DEFAULT_STR_SIZE are defined in types_max_str_size.h
        u32 num_frac_int = (u32)(num_frac * F32_DEC_FRAC_MULT);
        log_sized_dec_u32(num_frac_int, F32_DEC_FRAC_DEFAULT_STR_SIZE);
      }
    }
  }
  else
  {
    u8* dest = logs.buffer + logs.buffer_end_idx;
    dest[0] = 'b';
    dest[1] = 'i';
    dest[2] = 'g';

    logs.buffer_end_idx += 3u;
  }
}


void log_dec_f32(f32 num)
{
  u32 is_a_number = f32_is_a_number(num);
  if (is_a_number)
  {
    log_dec_f32_number(num);
  }
  else
  {
    log_dec_f32_nan_or_inf(num);
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Hexadecimal number logging
static const char hex_digits[] = "0123456789ABCDEF";

void log_sized_hex_s8 (s8  num, u32 nibble_to_write_count) { log_sized_hex_u64((u64)num,    nibble_to_write_count); }
void log_sized_hex_s16(s16 num, u32 nibble_to_write_count) { log_sized_hex_u64((u64)num,    nibble_to_write_count); }
void log_sized_hex_s32(s32 num, u32 nibble_to_write_count) { log_sized_hex_u64((u64)num,    nibble_to_write_count); }
void log_sized_hex_s64(s64 num, u32 nibble_to_write_count) { log_sized_hex_u64((u64)num,    nibble_to_write_count); }
void log_sized_hex_u8 (u8  num, u32 nibble_to_write_count) { log_sized_hex_u64(num,         nibble_to_write_count); }
void log_sized_hex_u16(u16 num, u32 nibble_to_write_count) { log_sized_hex_u64(num,         nibble_to_write_count); }
void log_sized_hex_u32(u32 num, u32 nibble_to_write_count) { log_sized_hex_u64(num,         nibble_to_write_count); }


void log_sized_hex_u64(u64 num, u32 nibble_to_write_count)
{
  u8* const num_str_start = logs.buffer + logs.buffer_end_idx;
  u8*       dest          = num_str_start + nibble_to_write_count;
  while (dest > num_str_start)
  {
    u8 nibble_idx = num & 0xF;
    
    dest -= 1;
    *dest = hex_digits[nibble_idx];
    
    num >>= 4;
  }

  logs.buffer_end_idx += nibble_to_write_count;
}


void log_sized_hex_f32(f32 num, u32 nibble_to_write_count) { log_sized_hex_u64(*(u32*)&num, nibble_to_write_count); }


void log_hex_s8 (s8  num) { log_sized_hex_u64((u64)num,    u32_nibble_count((u32)num));    }
void log_hex_s16(s16 num) { log_sized_hex_u64((u64)num,    u32_nibble_count((u32)num));    }
void log_hex_s32(s32 num) { log_sized_hex_u64((u64)num,    u32_nibble_count((u32)num));    }
void log_hex_s64(s64 num) { log_sized_hex_u64((u64)num,    u64_nibble_count((u64)num));    }
void log_hex_u8 (u8  num) { log_sized_hex_u64(num,         u32_nibble_count(num));         }
void log_hex_u16(u16 num) { log_sized_hex_u64(num,         u32_nibble_count(num));         }
void log_hex_u32(u32 num) { log_sized_hex_u64(num,         u32_nibble_count(num));         }
void log_hex_u64(u64 num) { log_sized_hex_u64(num,         u64_nibble_count(num));         }
void log_hex_f32(f32 num) { log_sized_hex_u64(*(u32*)&num, u32_nibble_count(*(u32*)&num)); }

#elif defined(_MSC_VER)
#  pragma warning(disable: 4206) // Disable "empty translation unit" warning
#endif // defined(LOGS_ENABLED) && (LOGS_ENABLED != 0)

