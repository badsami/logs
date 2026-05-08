#if defined(LOGS_ENABLED) && (LOGS_ENABLED != 0)
#include "logs.h"

#if defined(LOGS_OS_WINDOWS)
#  define _WIN32_WINNT 0x0501 // ATTACH_PARENT_PROCESS
#  include <Windows.h>
#elif defined(LOGS_OS_LINUX)
// #  include "linux_logs_syscalls.h"
#  define STDOUT_FD 1
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Helpers
static inline u32 open_file_output_ascii(const char* file_path)
{
#if defined(LOGS_OS_WINDOWS)
  const u32 SHARE_RDWR = FILE_SHARE_READ | FILE_SHARE_WRITE;
  HANDLE output = CreateFileA(file_path,        // lpFileName
                              FILE_APPEND_DATA, // dwDesiredAccess
                              SHARE_RDWR,       // dwShareMode
                              0,                // lpSecurityAttributes
                              OPEN_ALWAYS,      // dwCreationDisposition
                              0,                // dwFlagsAndAttributes
                              0);               // hTemplateFile
  return (u32)(u64)output;
#elif defined(LOGS_OS_LINUX)
  // TODO: O_ASYNC?
  #define O_RDWR	          00000002
  #define O_CREAT           00000100
  #define O_APPEND          00002000
  #define CREAT_PERMISSIONS 00000775
  
  const u64 flags = O_RDWR | O_CREAT | O_APPEND; 
  register u64         open_syscall_rax __asm__("rax") = 2;
  register const char* file_path_rdi    __asm__("rdi") = file_path;
  register u64         flags_rsi        __asm__("rsi") = flags;
  register u64         mode_rdx         __asm__("rdx") = CREAT_PERMISSIONS;
  s64 output;
  __asm__ __volatile__ ("syscall" :
                        "=a"(output) :
                        "r"(open_syscall_rax), "r"(file_path_rdi), "r"(flags_rsi), "r"(mode_rdx) :
                        "rcx", "r11", "memory");
  return (u32)(s32)output;
#endif
}

static inline void write_to_output(u32 output, const u8* data, u64 data_size)
{
#if defined(LOGS_OS_WINDOWS)
  HANDLE handle = (HANDLE)(u64)output;
  WriteFile(handle, data, (u32)data_size, 0, 0);
#elif defined(LOGS_OS_LINUX)
  register u64       write_syscall_rax __asm__("rax") = 1;
  register u32       output_rdi        __asm__("rdi") = output;
  register const u8* data_rsi          __asm__("rsi") = data;
  register u64       data_size_rdx     __asm__("rdx") = data_size;
  __asm__ __volatile__ ("syscall" : :
                        "r"(write_syscall_rax), "r"(output_rdi), "r"(data_rsi), "r"(data_size_rdx) :
                        "rcx", "r11", "memory");
#endif
}


static inline void logs_close_output(logs_output_idx output_idx)
{
  u32 output = logs.outputs[output_idx];

#if defined(LOGS_OS_WINDOWS)
  CloseHandle((HANDLE)(u64)output);
#elif defined(LOGS_OS_LINUX)
  register u64 close_syscall_rax __asm__("rax") = 3;
  register u64 output_rdi        __asm__("rdi") = logs.outputs[output_idx];
  __asm__ __volatile__ ("syscall" : :
                        "r"(close_syscall_rax), "r"(output_rdi) :
                        "rcx", "r11", "memory");
#endif

  logs.outputs[output_idx] = 0;
}




///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Global
struct logs logs =
{
  .buffer = {0},
  .outputs =
  {
#if defined(LOGS_OS_LINUX)
    // stdout is opened by default
    [LOGS_OUTPUT_CONSOLE] = STDOUT_FD,
#else
    [LOGS_OUTPUT_CONSOLE] = 0,
#endif
    [LOGS_OUTPUT_FILE]    = 0
  },
  .buffer_end_idx = 0
};




///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Output management
// Console output
void logs_open_console_output(void)
{
  if (logs.outputs[LOGS_OUTPUT_CONSOLE] == 0)
  {
#if defined(LOGS_OS_WINDOWS)
    const BOOL success    = AttachConsole(ATTACH_PARENT_PROCESS);
    const u32  last_error = GetLastError();
    if ((success == 0) && (last_error != ERROR_ACCESS_DENIED))
    {
      // There is no console to borrow, or something went wrong. Create a new console
      AllocConsole();
      SetConsoleTitleA("Logs");

      logs.console_original_output_code_page = 0;
    }
    else
    {
      // An existing console is attached
      logs.console_original_output_code_page = GetConsoleOutputCP();
    }

    SetConsoleOutputCP(CP_UTF8);

    const u32 SHARE_MODE = FILE_SHARE_READ | FILE_SHARE_WRITE;
    HANDLE output = CreateFileA("\\\\?\\CONOUT$", // lpFileName
                                GENERIC_WRITE,    // dwDesiredAccess
                                SHARE_MODE,       // dwShareMode
                                0,                // lpSecurityAttributes
                                OPEN_EXISTING,    // dwCreationDisposition
                                0,                // dwFlagsAndAttributes
                                0);               // hTemplateFile
    logs.outputs[LOGS_OUTPUT_CONSOLE] = (u32)(u64)output;

#elif defined(LOGS_OS_LINUX)
    // Since stdout is not opened nor handled by this process, its file descriptor is set and unset
    // to indicate whether it should be used or not, but it is otherwise left opened and never
    // closed nor re-opened
    logs.outputs[LOGS_OUTPUT_CONSOLE] = STDOUT_FD;
#endif
  }
}


// NOTE (Sami): this breaks Windows Terminal (the default Windows 11 console)
void logs_close_console_output(void)
{
  if (logs.outputs[LOGS_OUTPUT_CONSOLE] != 0)
  {
#if defined(LOGS_OS_WINDOWS)
    logs_close_output(LOGS_OUTPUT_CONSOLE);
    if (logs.console_original_output_code_page != 0)
    {
      SetConsoleOutputCP(logs.console_original_output_code_page);
    }
    
    // Free the console of this process
    FreeConsole();
#elif defined(LOGS_OS_LINUX)
    // stdout file descriptor is removed to indicate it should not be used, but it is never closed
    logs.outputs[LOGS_OUTPUT_CONSOLE] = 0;
#endif
  }
}


// File output
void logs_open_file_output_ascii(const char* file_path)
{
  if (logs.outputs[LOGS_OUTPUT_FILE] == 0)
  {
    u32 output = open_file_output_ascii(file_path);
    logs.outputs[LOGS_OUTPUT_FILE] = output;
  }
}


void logs_open_file_output_utf16(const char16* file_path)
{
  if (logs.outputs[LOGS_OUTPUT_FILE] == 0)
  {
#if defined(LOGS_OS_WINDOWS)
    const u32 SHARE_MODE = FILE_SHARE_READ | FILE_SHARE_WRITE;
    HANDLE output = CreateFileW(file_path,        // lpFileName
                                FILE_APPEND_DATA, // dwDesiredAccess
                                SHARE_MODE,       // dwShareMode
                                0,                // lpSecurityAttributes
                                OPEN_ALWAYS,      // dwCreationDisposition
                                0,                // dwFlagsAndAttributes
                                0);               // hTemplateFile
    logs.outputs[LOGS_OUTPUT_FILE] = (u32)(u64)output;
#elif defined(LOGS_OS_LINUX)
    u32 output = open_file_output_ascii((char*)file_path);
    logs.outputs[LOGS_OUTPUT_FILE] = output;
#endif
  }
}


void logs_close_file_output(void)
{
  if (logs.outputs[LOGS_OUTPUT_FILE] != 0)
  {
    logs_close_output(LOGS_OUTPUT_FILE);
  }
}


// All outputs
void logs_flush(void)
{
  // Trust that the caller knows the log buffer is not empty
  for (u64 i = 0; i < LOGS_OUTPUT_COUNT; i++)
  {
    u32 output = logs.outputs[i];
    if (output != 0)
    {
      write_to_output(output, logs.buffer, logs.buffer_end_idx);
    }
  }

  logs.buffer_end_idx = 0;
}


void logs_flush_to(logs_output_idx output_idx)
{
  u32 output = logs.outputs[output_idx];
  if (output != 0)
  {
    write_to_output(output, logs.buffer, logs.buffer_end_idx);
  }

  logs.buffer_end_idx = 0;
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Memory
u64 logs_buffer_remaining_bytes(void)
{
  s64 difference      = LOGS_BUFFER_SIZE - logs.buffer_end_idx;
  u64 remaining_bytes = (difference > 0ll) ? difference : 0ll;

  return remaining_bytes;
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Characters & strings logging
void log_utf8_character(char character)
{
  logs.buffer[logs.buffer_end_idx] = character;
  logs.buffer_end_idx += 1;
}


void log_utf16_character(char16 character)
{
  log_sized_utf16_str(&character, 1);
}


void log_sized_utf8_str(const char* str, u64 char_count)
{
  char* dest_u8 = (char*)(logs.buffer + logs.buffer_end_idx);

  const u64         u8_x8_count = char_count & ~7;
  const char* const str_x8_end  = str + u8_x8_count;
  const char* const str_end     = str + char_count;

  while (str < str_x8_end)
  {
    *(u64*)dest_u8 = *(u64*)str;

    dest_u8 += 8;
    str  += 8;
  }

  while (str < str_end)
  {
    *dest_u8 = *str;
    dest_u8 += 1;
    str  += 1;
  }

  logs.buffer_end_idx += char_count;
}


void log_sized_utf16_str(const char16* str, u64 char16_count)
{
  u8* dest = logs.buffer + logs.buffer_end_idx;
  while (char16_count != 0)
  {
    u32 unicode;
    u64 char16_read        = utf16_code_point_to_unicode(str, &unicode);
    u64 written_byte_count = unicode_to_utf8_code_point(unicode, dest);
    
    char16_count -= char16_read;
    str          += char16_read;
    dest         += written_byte_count;
  }

  logs.buffer_end_idx = dest - logs.buffer;
}


void log_null_terminated_utf8_str(const char* str)
{
  char character = *str;
  while (character != '\0')
  {
    logs.buffer[logs.buffer_end_idx] = character;
    logs.buffer_end_idx += 1;
    
    str       += 1;
    character = *str;
  }
}


void log_null_terminated_utf16_str(const char16* str)
{
  u8* dest = logs.buffer + logs.buffer_end_idx;
  while (*str != u'\0')
  {
    u32 unicode;
    u64 char16_read      = utf16_code_point_to_unicode(str, &unicode);
    u64 written_u8_count = unicode_to_utf8_code_point(unicode, dest);
    
    str  += char16_read;
    dest += written_u8_count;
  }

  logs.buffer_end_idx = dest - logs.buffer;
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Non-alphanumeric types logging
static const char* bool_str = "truefalse";

void log_bool(u64 boolean)
{
  const u64 is_false = (boolean == 0);
  const u64 offset   = is_false << 2; // 4 if (boolean == 0), 0 otherwise

  // Starts either at the 't' of "true" or the 'f' of "false"
  const char* bool_str_start = bool_str + offset;

  // length("true") = 4, length("false") = 5, is_false = 0 or 1
  const u64 char_count  = 4 + is_false;   
  log_sized_utf8_str(bool_str_start, char_count);
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Compounds logging
static const u64 unit_multipliers[] =
{
  1ull, // no unit multiplier
  1000ull, // Kilo (K)
  1000000ull, // Mega (M)
  1000000000ull, // Giga (G)
  1000000000000ull, // Tera (T)
  1000000000000000ull, // Peta (P)
  1000000000000000000ull // Exa (E)
};

static const char unit_prefixes[7] = {0, 'K', 'M', 'G', 'T', 'P', 'E'};

void log_byte_count_dec_unit(u64 byte_count)
{
  // Log the integer part
  const u64 digit_count          = u64_digit_count(byte_count); // in [1; 20]
  const u64 unit_idx             = (digit_count - 1) / 3; // in [0; 6]
  const u64 unit_mul             = unit_multipliers[unit_idx];
  const u64 int_byte_count       = byte_count / unit_mul; // has 1 to 3 digits
  const u64 int_byte_digit_count = digit_count - (unit_idx * 3);

  log_sized_dec_u64(int_byte_count, int_byte_digit_count);

  // Log the fractional part (if necessary)
  const u32 byte_count_ge_1000 = byte_count >= 1000;
  if (byte_count_ge_1000)
  {
    const u64 byte_count_remainder = byte_count - (int_byte_count * unit_mul);
    const u64 frac_byte_count      = (BYTE_COUNT_FRAC_DIV * byte_count_remainder) / unit_mul;
    log_character('.');
    log_sized_dec_u64(frac_byte_count, BYTE_COUNT_FRAC_SIZE);
  }

  // Log the unit prefix and the unit itself
  // ' ' [+ unit prefix] + 'B' = 2 mandatory + 1 optional characters
  u8* const dest              = logs.buffer + logs.buffer_end_idx;
  const u64 b_char_idx        = 1 + byte_count_ge_1000;
  const u64 suffix_char_count = 2 + byte_count_ge_1000;

  dest[0]          = ' ';
  dest[1]          = unit_prefixes[unit_idx]; // overwritten if unnecessary
  dest[b_char_idx] = 'B';

  logs.buffer_end_idx += suffix_char_count;
}


void log_byte_count_bin_unit(u64 byte_count)
{
  // Log the integer part
  const u64 msb_idx        = get_msb_1_bit_idx_u64(byte_count);
  const u64 prefix_idx     = msb_idx / 10;
  const u8  mul_shift      = (u8)(prefix_idx * 10);
  const u32 int_byte_count = (u32)(byte_count >> mul_shift);
  log_dec_u32(int_byte_count);

  // Log the fractional part (if necessary)
  const u32 byte_count_ge_1024 = byte_count >= 1024;
  if (byte_count_ge_1024)
  {
    const u64 byte_count_remainder = byte_count - (int_byte_count << mul_shift);
    const u64 unit_mul             = 1ull << mul_shift;
    const u64 frac_byte_count      = (BYTE_COUNT_FRAC_DIV * byte_count_remainder) / unit_mul;
    log_character('.');
    log_sized_dec_u64(frac_byte_count, BYTE_COUNT_FRAC_SIZE);
  }

  // Log the unit prefix and the unit itself
  // ' ' [+ unit prefix + 'i'] + 'B' = 2 mandatory + 2 optional characters
  u8* const dest              = logs.buffer + logs.buffer_end_idx;
  const u64 b_char_offset     = byte_count_ge_1024 * 2;
  const u64 suffix_char_count = 2 + b_char_offset;
  const u64 i_char_idx        = 1 + byte_count_ge_1024;
  const u64 b_char_idx        = 1 + b_char_offset;

  dest[0]          = ' ';
  dest[1]          = unit_prefixes[prefix_idx]; // overwritten if unnecessary
  dest[i_char_idx] = 'i'; // overwritten if unnecessary
  dest[b_char_idx] = 'B';

  logs.buffer_end_idx += suffix_char_count;
}


void log_os_api_error(u32 error_code)
{
#if defined(LOGS_OS_WINDOWS)
  const DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

  // Can be obtained either from winnt.h, or through "Windows Language Code Identifier (LCID)
  // Reference", version 16.0 (23rd April 2024), page 14:
  // https://winprotocoldoc.z19.web.core.windows.net/MS-LCID/[MS-LCID].pdf#page=14
  const WORD en_us_lang_id = 0x0409;

  // None of the functions appending content to the logs buffer make sure there is enough space to
  // write to. Lie about the available space in the logs buffer to remain consistent
  const DWORD max_bytes = 64000; // maximum allowed by FormatMessage()

  log_literal_str("Windows API error ");
  log_dec_u32(error_code);
  log_literal_str(": ");

  char* dest = (char*)(logs.buffer + logs.buffer_end_idx);

  DWORD char_written = FormatMessageA(flags,         // dwFlags
                                      0,             // lpSource
                                      error_code,    // dwMessageId
                                      en_us_lang_id, // dwLanguageId
                                      dest,          // lpBuffer
                                      max_bytes,     // nSize
                                      0);            // Arguments
  if (char_written == 0)
  {
    // Something went wrong, fallback
    log_literal_str("couldn't get error description)");
  }
  else
  {
    // Messages finish with a carriage return + linefeed. Both are removed
    logs.buffer_end_idx += char_written - 2;
  }
#elif defined(LOGS_OS_LINUX)
#  include "linux_errno_to_str.inl"
  const char* error_str = linux_errno_to_str[error_code];
  log_literal_str("Linux API error ");
  log_dec_u32(error_code);
  log_literal_str(": ");
  log_null_terminated_ascii_str(error_str);
#endif
}




///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Numbers logging
// Binary
void log_sized_bin_s8 (s8  num, u64 bit_to_write_count) { log_sized_bin_u64((u64)num, bit_to_write_count); }
void log_sized_bin_s16(s16 num, u64 bit_to_write_count) { log_sized_bin_u64((u64)num, bit_to_write_count); }
void log_sized_bin_s32(s32 num, u64 bit_to_write_count) { log_sized_bin_u64((u64)num, bit_to_write_count); }
void log_sized_bin_s64(s64 num, u64 bit_to_write_count) { log_sized_bin_u64((u64)num, bit_to_write_count); }
void log_sized_bin_u8 (u8  num, u64 bit_to_write_count) { log_sized_bin_u64(num,      bit_to_write_count); }
void log_sized_bin_u16(u16 num, u64 bit_to_write_count) { log_sized_bin_u64(num,      bit_to_write_count); }
void log_sized_bin_u32(u32 num, u64 bit_to_write_count) { log_sized_bin_u64(num,      bit_to_write_count); }


void log_sized_bin_u64(u64 num, u64 bit_to_write_count)
{
  u8* const num_str_start = logs.buffer + logs.buffer_end_idx;
  u8*       dest          = num_str_start + bit_to_write_count;
  while (dest > num_str_start)
  {
    u8 bit = num & 1;
    
    dest -= 1;
    *dest = '0' + bit;
    
    num >>= 1;
  }

  logs.buffer_end_idx += bit_to_write_count;
}


void log_sized_bin_f32(f32 num, u64 bit_to_write_count) { log_sized_bin_u64(*(u32*)(&num), bit_to_write_count); }


void log_bin_s8 (s8  num) { log_sized_bin_u64((u64)num,      u32_bit_count((u32)num));    }
void log_bin_s16(s16 num) { log_sized_bin_u64((u64)num,      u32_bit_count((u32)num));    }
void log_bin_s32(s32 num) { log_sized_bin_u64((u64)num,      u32_bit_count((u32)num));    }
void log_bin_s64(s64 num) { log_sized_bin_u64((u64)num,      u64_bit_count((u64)num));    }
void log_bin_u8 (u8  num) { log_sized_bin_u64(num,           u32_bit_count(num));         }
void log_bin_u16(u16 num) { log_sized_bin_u64(num,           u32_bit_count(num));         }
void log_bin_u32(u32 num) { log_sized_bin_u64(num,           u32_bit_count(num));         }
void log_bin_u64(u64 num) { log_sized_bin_u64(num,           u64_bit_count(num));         }
void log_bin_f32(f32 num) { log_sized_bin_u64(*(u32*)(&num), u32_bit_count(*(u32*)&num)); }


// Decimal number logging
void log_sized_dec_s8 (s8  num, u64 digit_to_write_count) { log_sized_dec_u64((u64)num, digit_to_write_count); }
void log_sized_dec_s16(s16 num, u64 digit_to_write_count) { log_sized_dec_u64((u64)num, digit_to_write_count); }
void log_sized_dec_s32(s32 num, u64 digit_to_write_count) { log_sized_dec_u64((u64)num, digit_to_write_count); }


void log_sized_dec_s64(s64 num, u64 digit_to_write_count)
{
  u64 is_neg  = num < 0;
  u64 pos_num = is_neg ? -num : num;

  logs.buffer[logs.buffer_end_idx] = '-'; // overwritten if unnecessary
  logs.buffer_end_idx += is_neg;
  
  log_sized_dec_u64(pos_num, digit_to_write_count);
}


void log_sized_dec_u8 (u8  num, u64 digit_to_write_count) { log_sized_dec_u64(num, digit_to_write_count); }
void log_sized_dec_u16(u16 num, u64 digit_to_write_count) { log_sized_dec_u64(num, digit_to_write_count); }
void log_sized_dec_u32(u32 num, u64 digit_to_write_count) { log_sized_dec_u64(num, digit_to_write_count); }


void log_sized_dec_u64(u64 num, u64 digit_to_write_count)
{
  u8* const num_str_start = logs.buffer + logs.buffer_end_idx;
  u8*       dest          = num_str_start + digit_to_write_count;
  while (dest > num_str_start)
  {
    u64 quotient = num / 10;
    u8  digit    = (u8)(num - (quotient * 10));

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

void log_sized_dec_f32_number(f32 num, u64 frac_digit_to_write_count)
{
  u32 is_neg = num < 0.f;
  
  logs.buffer[logs.buffer_end_idx] = '-'; // overwritten if unnecessary
  logs.buffer_end_idx += is_neg;

  // Absolute values equal or greater to 8 388 608 are likely better represented as a s32 or s64
  // than as a 32-bit floating-point value (thereafter referred to as "f32"), for two reasons:
  // 
  // 1. Range of values: f32 values can be as large as +/- 3.4 x 10^38. I've never needed to
  // represent values that large as a f32. If they are ever necessary to you, I believe s32 and s64
  // are better fits because they can represent values up to +/- 2.15 x 10^9 and +/- 9.22 x 10^18
  // respectively, which might cover your needs. Otherwise, changing scale (i.e. meter to lightyear)
  // might be a better option because of reason #2.
  // 
  // 2. Precision: starting at 8 388 608 and beyond, f32 values cannot have a fractional part.
  // Starting at 16 777 216 and beyond, adding 1 to a f32 value doesn't change it. The fractional
  // part is not very significant at this magnitude, but s32 and s64 retain a precision of 1 on the
  // full range of values they cover.
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
      // The same way values I've never needed to represent very large f32 values, I believe 
      // representing values under 0.000001 (without changing units) should never be needed
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
        // F32_DEC_FRAC_MAX_STR_SIZE is defined in logs.h
        u64 num_frac_digit_to_write_count = (frac_digit_to_write_count < F32_DEC_FRAC_MAX_STR_SIZE) ?
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

    logs.buffer_end_idx += 3;
  }
}


void log_sized_dec_f32(f32 num, u64 frac_size)
{
  u64 is_a_number = f32_is_a_number(num);
  if (is_a_number)
  {
    log_sized_dec_f32_number(num, frac_size);
  }
  else
  {
    log_dec_f32_nan_or_inf(num);
  }
}


void log_dec_s8 (s8  num) { log_dec_s32(num); }
void log_dec_s16(s16 num) { log_dec_s32(num); }


void log_dec_s32(s32 num)
{
  u32 is_neg  = num < 0ll;
  u32 pos_num = (u32)(is_neg ? -num : num);

  u8* num_str_start        = logs.buffer + logs.buffer_end_idx;
  u64 digit_to_write_count = u32_digit_count(pos_num);
  u64 char_to_write_count  = digit_to_write_count + is_neg;
  u8* dest                 = num_str_start + char_to_write_count;
  
  *num_str_start = '-'; // will be overwritten if not needed
  num_str_start += is_neg;
  while (dest > num_str_start)
  {
    u32 quotient = pos_num / 10;
    u8  digit    = (u8)(pos_num - (quotient * 10u));

    dest -= 1;
    *dest = '0' + digit;
    
    pos_num = quotient;
  }
  
  logs.buffer_end_idx += char_to_write_count;
}


void log_dec_s64(s64 num)
{
  u64 is_neg  = num < 0ll;
  u64 pos_num = (u64)(is_neg ? -num : num);

  u8* const num_str_start        = logs.buffer + logs.buffer_end_idx;
  u64       digit_to_write_count = u64_digit_count(pos_num);
  u64       char_to_write_count  = digit_to_write_count + is_neg;
  u8*       dest                 = num_str_start + char_to_write_count;
  
  *num_str_start = '-'; // will be overwritten if not needed
  while (dest > num_str_start)
  {
    u64 quotient = pos_num / 10;
    u8  digit    = (u8)(pos_num - (quotient * 10));

    dest -= 1;
    *dest = '0' + digit;
    
    pos_num = quotient;
  }
  
  logs.buffer_end_idx += char_to_write_count;
}


void log_dec_u8 (u8  num) { log_sized_dec_u64(num, u32_digit_count(num)); }
void log_dec_u16(u16 num) { log_sized_dec_u64(num, u32_digit_count(num)); }
void log_dec_u32(u32 num) { log_sized_dec_u64(num, u32_digit_count(num)); }
void log_dec_u64(u64 num) { log_sized_dec_u64(num, u64_digit_count(num)); }


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

  logs.buffer_end_idx += (u64)(dest - num_str_start);
}


void log_dec_f32_number(f32 num)
{
  u64 is_neg = num < 0.f;
  
  logs.buffer[logs.buffer_end_idx] = '-'; // overwritten if unnecessary
  logs.buffer_end_idx += is_neg;

  // Absolute values equal or greater than 8 388 608 are likely better represented as a s32 or s64
  // than as a 32-bit floating-point value (thereafter referred to as "f32"), for two reasons:
  // 
  // 1. Range of values: f32 values can be as large as +/- 3.4 x 10^38. I've never needed to
  // represent values that large as a f32. If they are ever necessary to you, I believe s32 and s64
  // are better fits because they can represent values up to +/- 2.15 x 10^9 and +/- 9.22 x 10^18
  // respectively, which might cover your needs. Otherwise, changing scale (i.e. meter to lightyear)
  // might be a better option because of reason #2.
  // 
  // 2. Precision: starting at 8 388 608 and beyond, f32 values cannot have a fractional part.
  // Starting at 16 777 216 and beyond, adding 1 to a f32 value doesn't change it. The fractional
  // part is not very significant at this magnitude, but s32 and s64 retain a precision of 1 on the
  // full range of values they cover.
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
      // The same way values I've never needed to represent very large f32 values, I believe 
      // representing values under 0.000001 (without changing units) should never be needed
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

        // F32_DEC_FRAC_MULT and F32_DEC_FRAC_DEFAULT_STR_SIZE are defined in logs.h
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

    logs.buffer_end_idx += 3;
  }
}


void log_dec_f32(f32 num)
{
  u64 is_a_number = f32_is_a_number(num);
  if (is_a_number)
  {
    log_dec_f32_number(num);
  }
  else
  {
    log_dec_f32_nan_or_inf(num);
  }
}


// Hexadecimal
static const char hex_digits[] = "0123456789ABCDEF";

void log_sized_hex_s8 (s8  num, u64 nibble_to_write_count) { log_sized_hex_u64((u64)num, nibble_to_write_count); }
void log_sized_hex_s16(s16 num, u64 nibble_to_write_count) { log_sized_hex_u64((u64)num, nibble_to_write_count); }
void log_sized_hex_s32(s32 num, u64 nibble_to_write_count) { log_sized_hex_u64((u64)num, nibble_to_write_count); }
void log_sized_hex_s64(s64 num, u64 nibble_to_write_count) { log_sized_hex_u64((u64)num, nibble_to_write_count); }
void log_sized_hex_u8 (u8  num, u64 nibble_to_write_count) { log_sized_hex_u64(num,      nibble_to_write_count); }
void log_sized_hex_u16(u16 num, u64 nibble_to_write_count) { log_sized_hex_u64(num,      nibble_to_write_count); }
void log_sized_hex_u32(u32 num, u64 nibble_to_write_count) { log_sized_hex_u64(num,      nibble_to_write_count); }


void log_sized_hex_u64(u64 num, u64 nibble_to_write_count)
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


void log_sized_hex_f32(f32 num, u64 nibble_to_write_count) { log_sized_hex_u64(*(u32*)&num, nibble_to_write_count); }


void log_hex_s8 (s8  num) { log_sized_hex_u64((u64)num,    u32_nibble_count((u32)num));    }
void log_hex_s16(s16 num) { log_sized_hex_u64((u64)num,    u32_nibble_count((u32)num));    }
void log_hex_s32(s32 num) { log_sized_hex_u64((u64)num,    u32_nibble_count((u32)num));    }
void log_hex_s64(s64 num) { log_sized_hex_u64((u64)num,    u64_nibble_count((u64)num));    }
void log_hex_u8 (u8  num) { log_sized_hex_u64(num,         u32_nibble_count(num));         }
void log_hex_u16(u16 num) { log_sized_hex_u64(num,         u32_nibble_count(num));         }
void log_hex_u32(u32 num) { log_sized_hex_u64(num,         u32_nibble_count(num));         }
void log_hex_u64(u64 num) { log_sized_hex_u64(num,         u64_nibble_count(num));         }
void log_hex_f32(f32 num) { log_sized_hex_u64(*(u32*)&num, u32_nibble_count(*(u32*)&num)); }




///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Utilities
u64 lzcnt32(u32 num)
{
#if defined(_MSC_VER)
  return __lzcnt(num);
#elif defined(__clang__) || defined(__GNUC__)
  return __builtin_ia32_lzcnt_u32(num);
#endif
}


u64 lzcnt64(u64 num)
{
#if defined(_MSC_VER)
  return __lzcnt64(num);
#elif defined(__clang__) || defined(__GNUC__)
  return __builtin_ia32_lzcnt_u64(num);
#endif
}


u32 pdep32(u32 a, u32 mask)
{
#if defined(_MSC_VER)
  return _pdep_u32(a, mask);
#elif defined(__clang__) || defined(__GNUC__)
  return __builtin_ia32_pdep_si(a, mask);
#endif
}


u32 bswap32(u32 a)
{
#if defined(_MSC_VER)
  // Should generate a bswap instruction
  return (a              << 24) |
         (a              >> 24) |
         ((a & 0xFF00)   << 8) |
         ((a & 0xFF0000) >> 8);
#elif defined(__clang__) || defined(__GNUC__)
  return __builtin_bswap32(a);
#endif
}


u64 get_msb_1_bit_idx_u32(u32 num)
{
  return 31 ^ lzcnt32(num | 1);
}


u64 get_msb_1_bit_idx_u64(u64 num)
{
  return 63 ^ lzcnt64(num | 1);
}


// Numerals count in a number
u64 u32_digit_count(u32 num)
{
  // Similar to https://commaok.xyz/post/lookup_tables/, but:
  // - lzcnt replaces bsr
  // - table is reversed
  // - an extra entry is added for num = 0, so this function returns 1
  // - the bitwise OR and substraction performed by int_log2() no longer happen
  static const u64 offsets[33] =
  {
    42949672960ull, 42949672960ull,                                 // (10 << 32)
    41949672960ull, 41949672960ull, 41949672960ull,                 // (10 << 32) - 1000000000
    38554705664ull, 38554705664ull, 38554705664ull,                 // (9  << 32) - 100000000
    34349738368ull, 34349738368ull, 34349738368ull, 34349738368ull, // (8  << 32) - 10000000
    30063771072ull, 30063771072ull, 30063771072ull,                 // (7  << 32) - 1000000
    25769703776ull, 25769703776ull, 25769703776ull,                 // (6  << 32) - 100000
    21474826480ull, 21474826480ull, 21474826480ull, 21474826480ull, // (5  << 32) - 10000
    17179868184ull, 17179868184ull, 17179868184ull,                 // (4  << 32) - 1000
    12884901788ull, 12884901788ull, 12884901788ull,                 // (3  << 32) - 100
    8589934582ull,  8589934582ull,  8589934582ull,                  // (2  << 32) - 10
    4294967296ull,                                                  // (1  << 32)
    4294967296ull                                                   // (1  << 32)
  };

  u64 lzcnt       = lzcnt32(num);
  u64 offset      = offsets[lzcnt];
  u64 digit_count = (num + offset) >> 32;

  return digit_count;
}


u64 u64_digit_count(u64 num)
{
  // https://lemire.me/blog/2025/01/07/counting-the-digits-of-64-bit-integers/
  static u64 thresholds[19] =
  {
    9ull,
    99ull,
    999ull,
    9999ull,
    99999ull,
    999999ull,
    9999999ull,
    99999999ull,
    999999999ull,
    9999999999ull,
    99999999999ull,
    999999999999ull,
    9999999999999ull,
    99999999999999ull,
    999999999999999ull,
    9999999999999999ull,
    99999999999999999ull,
    999999999999999999ull,
    9999999999999999999ull
  };
  u64 msb_idx       = get_msb_1_bit_idx_u64(num);
  u64 threshold_idx = (19 * msb_idx) >> 6;
  u64 threshold     = thresholds[threshold_idx];
  u64 is_greater    = num > threshold;
  u64 digit_count   = threshold_idx + is_greater + 1;

  return digit_count;
}


u64 u32_bit_count(u32 num)
{
  return get_msb_1_bit_idx_u32(num) + 1;
}


u64 u64_bit_count(u64 num)
{
  return get_msb_1_bit_idx_u64(num) + 1;
}


u64 u32_nibble_count(u32 num)
{
  u64 msb_idx      = get_msb_1_bit_idx_u32(num);
  u64 nibble_count = 1 + (msb_idx >> 2);

  return nibble_count;
}


u64 u64_nibble_count(u64 num)
{
  u64 msb_idx      = get_msb_1_bit_idx_u64(num);
  u64 nibble_count = 1 + (msb_idx >> 2);
  
  return nibble_count;
}


u64 f32_is_a_number(f32 num)
{
  const u32 EXPONENT_ALL_ONE = 0x7F800000;
  
  u32 num_bits = *(u32*)&num;
  
  return (num_bits & EXPONENT_ALL_ONE) != EXPONENT_ALL_ONE;
}


u64 utf16_code_point_to_unicode(const u16* utf16, u32* unicode)
{
  const u16 range = 0xDFFF - 0xD800;
  u16 offset = utf16[0] - 0xD800;
  if (offset > range)
  {
    *unicode = utf16[0];
    return 1;
  }
  else
  {
    u32 lo = utf16[0] & 0x3FF;
    u32 hi = (utf16[1] & 0x3FF) << 10;
    
    *unicode = (hi | lo) + 0x10000;
    return 2;
  }
}


static const u8 lzcnt_to_utf8_byte_count[33] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  4, 4, 4, 4, 4,
  3, 3, 3, 3, 3,
  2, 2, 2, 2,
  1, 1, 1, 1, 1, 1, 1,
  0
};

static const u32 high_order_bits[] =
{
  0x0,
  0x0,
  0x80C0, // 0b1000000011000000
  0x8080E0, // 0b100000001000000011100000
  0x808080F0, // 0b10000000100000001000000011110000
};

static const u32 pdep_mask[] =
{
  0x0,
  0xFF000000, // 0b11111111000000000000000000000000
  0x1F3F0000, // 0b00011111001111110000000000000000
  0x0F3F3F00, // 0b00001111001111110011111100000000
  0x073F3F3F  // 0b00000111001111110011111100111111
};

u64 unicode_to_utf8_code_point(u32 unicode, u8* utf8)
{
  // lzcnt | byte_count | Unicode
  // ------|------------|---------------------------
  // 31-25 | 1          | 0wwwwwww
  // 24-21 | 2          | 00000xxx xxwwwwww
  // 20-16 | 3          | yyyyxxxx xxwwwwww
  // 15-11 | 4          | 000zzzyy yyyyxxxx xxwwwwww
  // 10- 0 | 0          | Invalid unicode
  u64 lzcnt      = lzcnt32(unicode);
  u64 byte_count = lzcnt_to_utf8_byte_count[lzcnt];

  // byte_count |              deposited              |         bswap32(deposited)
  // -----------|-------------------------------------|------------------------------------
  // 1          | wwwwwwww 00000000 00000000 00000000 | 00000000 00000000 00000000 wwwwwwww
  // 2          | 000xxxxx 00wwwwww 00000000 00000000 | 00000000 00000000 00wwwwww 000xxxxx
  // 3          | 0000yyyy 00xxxxxx 00wwwwww 00000000 | 00000000 00wwwwww 00xxxxxx 0000yyyy
  // 4          | 00000zzz 00yyyyyy 00xxxxxx 00wwwwww | 00wwwwww 00xxxxxx 00yyyyyy 00000zzz
  u32 deposited = pdep32(unicode, pdep_mask[byte_count]);
  u32 bswapped  = bswap32(deposited);

  // byte_count |         bswap32(deposited)          |               encoded
  // -----------|-------------------------------------|------------------------------------
  // 1          | 00000000 00000000 00000000 wwwwwwww | 00000000 00000000 00000000 wwwwwwww
  // 2          | 00000000 00000000 00wwwwww 000xxxxx | 00000000 00000000 10wwwwww 110xxxxx
  // 3          | 00000000 00wwwwww 00xxxxxx 0000yyyy | 00000000 10wwwwww 10xxxxxx 1110yyyy
  // 4          | 00wwwwww 00xxxxxx 00yyyyyy 00000zzz | 10wwwwww 10xxxxxx 10yyyyyy 11110zzz
  u32 high_bits = high_order_bits[byte_count];
  u32 encoded   = high_bits | bswapped;

  *(u32*)utf8 = encoded;

  return byte_count;
}

#elif defined(_MSC_VER)
#  pragma warning(disable: 4206) // Disable "empty translation unit" warning
#endif // defined(LOGS_ENABLED) && (LOGS_ENABLED != 0)
