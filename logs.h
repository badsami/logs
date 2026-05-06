// This logging API offers a way to control:
// 1. how various data types are formatted to strings and appended to a buffer of fixed size
// 2. when to flush/write the content of the buffer, which empties the buffer
// 3. which outputs (console or file, currently) the buffer is flushed/written to
//
// Control over logs memory management is deferred to users and provided through:
// - global and custommizable log buffer compile-time size
// - global access to the logs struct instance
// - logs_buffer_remaining_bytes()
// - logs_flush()
//
// None of the functions below check whether enough space is available in the buffer before
// appending content to it. It is advised to tweak LOGS_BUFFER_SIZE to a value that's appropriate
// to your use-case (typically to the size of your biggest "chunk" of logs), either through compiler
// flags (-DLOGS_BUFFER_SIZE=<your size>) or by changing its default size (see below).
//
// Functions with a name starting with "logs_" (e.g. logs_open_console_output()) manage the general
// logging context. Function with a name starting with "log_" (e.g. log_dec_u32()) format and append
// data to the global logs buffer. Generic macros are provided to make writing logging code a little
// easier (e.g. log_dec_num() can be passed a signed 16-bit integer just as well as a 32-bit float)
#pragma once

#include "detect_compilation_os.h"
#include "types.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Data
#if !defined(LOGS_BUFFER_SIZE) || (LOGS_BUFFER_SIZE == 0)
#  define LOGS_BUFFER_SIZE 4096
#endif

// Index of available outputs in logs.outputs
enum logs_output_idx
{
  LOGS_OUTPUT_CONSOLE = 0,
  LOGS_OUTPUT_FILE    = 1,
  
  LOGS_OUTPUT_COUNT
};
typedef enum logs_output_idx logs_output_idx;

struct logs
{
  // Characters storage, encoded as UTF-8
  u8 buffer[LOGS_BUFFER_SIZE];

  // Output handles
  u32 outputs[LOGS_OUTPUT_COUNT];

  // Index past the last character written to the buffer
  u64 buffer_end_idx;

#if defined(LOGS_OS_WINDOWS)
  // If the console used to output logs is borrowed, restore its original output code page
  // when logs_close_console_output() is called. This is only used on Windows, whose terminal has
  // a fixed and predictable API. On Linux, the active terminal (if any) is expected to use UTF-8
  u32 console_original_output_code_page;
#endif
};


#if defined(LOGS_ENABLED) && (LOGS_ENABLED != 0) 
extern struct logs logs;


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Output management
// Open the console output, where logs will be written
void logs_open_console_output(void);

// Close the log console output, where logs will no longer be written
void logs_close_console_output(void);

// Open a file to append the logs to. file_path is an ASCII-encoded relative or absolute path (as in
// between "" quotes) that must be null-terminated. If the file exists, logs will be appended to the
// end of the file. If the file doesn't already exist, it is created
void logs_open_file_output_ascii(const char* file_path);

#if defined(LOGS_OS_LINUX)
// Open a file to append the logs to. file_path is an UTF8-encoded relative or absolute path (as in
// between u8"" quotes) that must be null-terminated. If the file exists, logs will be appended to
// the end of the file. If the file doesn't already exist, it is created.
//
// Windows only handles ASCII and UTF-16 paths, this is only defined for Linux
#  define logs_open_file_output_utf8(file_path) logs_open_file_output_ascii(file_path)
#  define logs_open_file_output(file_path)      logs_open_file_output_ascii(file_path)

#elif defined(LOGS_OS_WINDOWS)
// Open a file to append the logs to. file_path is an UTF16-encoded relative or absolute path (as in
// between u"" quotes, or L"" on Windows only) that must be null-terminated. If the file exists,
// logs will be appended to the end of the file. If the file doesn't already exist, it is created.
//
// Linux and UTF-16 don't play well together, this is only defined for Windows
  void logs_open_file_output_utf16(const char16* file_path);
#  define logs_open_file_output(file_path) logs_open_file_output_utf16(file_path)
#endif

// Close the log file output, where logs will no longer be written
void logs_close_file_output(void);

// Write the content of the log buffer to an output and set the log buffer end index to 0
void logs_flush_to(logs_output_idx output_idx);

// Write the content of the log buffer to all open outputs and set the log buffer end index to 0
void logs_flush(void);


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Memory
// Get the number of remaining available space in logs.buffer, in bytes
u64 logs_buffer_remaining_bytes(void);


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Characters & strings logging
// Append a single ASCII character ('a') to the log buffer.
// ASCII characters are encoded the same way as UTF-8
#define log_ascii_char(char_character) log_utf8_character(char_character)

// Append a single UTF-8 code point ('\xC3') to the log buffer
void log_utf8_character(char character);

// Append a single UTF-16 unit ('\u732B') to the log buffer
void log_utf16_character(char16 character);

// In C, an ASCII or UTF-8 character literal is an int by default
#define log_character(character)        \
  _Generic((character),                 \
           u8:     log_utf8_character,  \
           char:   log_utf8_character,  \
           int:    log_utf8_character,  \
           char16: log_utf16_character) \
          (character)

// Append a chain of ASCII-encoded characters ("abc") of predetermined to the log buffer.
// ASCII characters are encoded the same way as UTF-8
#define log_sized_ascii_str(str, char_count) log_sized_utf8_str(str, char_count)

// Append a chain of UTF-8-encoded characters (u8"Fluß") of predetermined size to the log buffer
void log_sized_utf8_str(const char* str, u64 char_count);

// Append a chain of UTF-16-encoded characters (u"çéÖ", L"çéÖ") of predetermined size to the
// log buffer
void log_sized_utf16_str(const char16* str, u64 char16_count);

#define log_sized_str(str, count)              \
  _Generic((str),                              \
           char*:         log_sized_utf8_str,  \
           const char*:   log_sized_utf8_str,  \
           u8*:           log_sized_utf8_str,  \
           const u8*:     log_sized_utf8_str,  \
           char16*:       log_sized_utf16_str, \
           const char16*: log_sized_utf16_str) \
          (str, count)

// Append a null-termianted chain of ASCII-encoded characters ("abc") to the log buffer.
// ASCII characters are encoded the same way as UTF-8
#define log_null_terminated_ascii_str(char_character) log_null_terminated_utf8_str(char_character)


// Append a null-terminated chain of UTF-8-encoded characters (u8"Fluß") to the log buffer
void log_null_terminated_utf8_str(const char* str);

// Append a null-terminated chain of UTF-16-encoded characters (u"çéÖ", or L"çéÖ" on Windows) to the
// log buffer
void log_null_terminated_utf16_str(const char16* str);

#define log_null_terminated_str(str)                     \
  _Generic((str),                                        \
           char*:         log_null_terminated_utf8_str,  \
           const char*:   log_null_terminated_utf8_str,  \
           u8*:           log_null_terminated_utf8_str,  \
           const u8*:     log_null_terminated_utf8_str,  \
           char16*:       log_null_terminated_utf16_str, \
           const char16*: log_null_terminated_utf16_str) \
          (str)


// Append a literal string ("abc", u8"Fluß", L"çéÖ") whose size can be determined at
// compile-time to the log buffer, without its final null terminator
#define log_literal_str(str)                   \
  _Generic((str),                              \
           char*:         log_sized_utf8_str,  \
           const char*:   log_sized_utf8_str,  \
           u8*:           log_sized_utf8_str,  \
           const u8*:     log_sized_utf8_str,  \
           char16*:       log_sized_utf16_str, \
           const char16*: log_sized_utf16_str) \
           (str, (sizeof(str) - sizeof(str[0])) / sizeof(str[0]))


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Numbers logging
// Binary
#define U8_MAX_BIN_STR_SIZE   8
#define U16_MAX_BIN_STR_SIZE 16
#define U32_MAX_BIN_STR_SIZE 32
#define U64_MAX_BIN_STR_SIZE 64
#define S8_MAX_BIN_STR_SIZE   8
#define S16_MAX_BIN_STR_SIZE 16
#define S32_MAX_BIN_STR_SIZE 32
#define S64_MAX_BIN_STR_SIZE 64
#define F32_MAX_BIN_STR_SIZE 32

void log_sized_bin_s8 (s8  num, u64 bit_to_write_count);
void log_sized_bin_s16(s16 num, u64 bit_to_write_count);
void log_sized_bin_s32(s32 num, u64 bit_to_write_count);
void log_sized_bin_s64(s64 num, u64 bit_to_write_count);
void log_sized_bin_u8 (u8  num, u64 bit_to_write_count);
void log_sized_bin_u16(u16 num, u64 bit_to_write_count);
void log_sized_bin_u32(u32 num, u64 bit_to_write_count);
void log_sized_bin_u64(u64 num, u64 bit_to_write_count);
void log_sized_bin_f32(f32 num, u64 bit_to_write_count);

#define log_sized_bin_num(num, bit_to_write_count) \
  _Generic((num),                                  \
           s8:  log_sized_bin_s8,                  \
           s16: log_sized_bin_s16,                 \
           s32: log_sized_bin_s32,                 \
           s64: log_sized_bin_s64,                 \
           u8:  log_sized_bin_u8,                  \
           u16: log_sized_bin_u16,                 \
           u32: log_sized_bin_u32,                 \
           u64: log_sized_bin_u64,                 \
           f32: log_sized_bin_f32)                 \
          (num, bit_to_write_count)

void log_bin_s8 (s8  num);
void log_bin_s16(s16 num);
void log_bin_s32(s32 num);
void log_bin_s64(s64 num);
void log_bin_u8 (u8  num);
void log_bin_u16(u16 num);
void log_bin_u32(u32 num);
void log_bin_u64(u64 num);
void log_bin_f32(f32 num);

#define log_bin_num(num)     \
  _Generic((num),            \
           s8:  log_bin_s8,  \
           s16: log_bin_s16, \
           s32: log_bin_s32, \
           s64: log_bin_s64, \
           u8:  log_bin_u8,  \
           u16: log_bin_u16, \
           u32: log_bin_u32, \
           u64: log_bin_u64, \
           f32: log_bin_f32) \
          (num)

// Decimal
#define U8_MAX_DEC_STR_SIZE            3
#define U16_MAX_DEC_STR_SIZE           5
#define U32_MAX_DEC_STR_SIZE          10
#define U64_MAX_DEC_STR_SIZE          20
#define S8_MAX_DEC_STR_SIZE            4
#define S16_MAX_DEC_STR_SIZE           6
#define S32_MAX_DEC_STR_SIZE          11
#define S64_MAX_DEC_STR_SIZE          20
#define F32_DEC_FRAC_DEFAULT_STR_SIZE  6
#define F32_DEC_FRAC_MAX_STR_SIZE      9

// Should be a 1 followed by F32_DEC_FRAC_DEFAULT_STR_SIZE zeros
#define F32_DEC_FRAC_MULT 1000000.f

// Sign + integer part + period + fractional part
#define F32_MAX_DEC_STR_SIZE (1 + U32_MAX_DEC_STR_SIZE + 1 + F32_DEC_FRAC_MAX_STR_SIZE)


void log_sized_dec_s8 (s8  num, u64 digit_to_write_count);
void log_sized_dec_s16(s16 num, u64 digit_to_write_count);
void log_sized_dec_s32(s32 num, u64 digit_to_write_count);
void log_sized_dec_s64(s64 num, u64 digit_to_write_count);
void log_sized_dec_u8 (u8  num, u64 digit_to_write_count);
void log_sized_dec_u16(u16 num, u64 digit_to_write_count);
void log_sized_dec_u32(u32 num, u64 digit_to_write_count);
void log_sized_dec_u64(u64 num, u64 digit_to_write_count);

void log_sized_dec_f32_number(f32 num, u64 frac_digit_to_write_count);
void log_sized_dec_f32(f32 num, u64 frac_digit_to_write_count);

#define log_sized_dec_num(num, digit_to_write_count) \
  _Generic((num),                               \
           s8:  log_sized_dec_s8,               \
           s16: log_sized_dec_s16,              \
           s32: log_sized_dec_s32,              \
           s64: log_sized_dec_s64,              \
           u8:  log_sized_dec_u8,               \
           u16: log_sized_dec_u16,              \
           u32: log_sized_dec_u32,              \
           u64: log_sized_dec_u64,              \
           f32: log_sized_dec_f32)              \
          (num, digit_to_write_count)

void log_dec_s8 (s8  num);
void log_dec_s16(s16 num);
void log_dec_s32(s32 num);
void log_dec_s64(s64 num);
void log_dec_u8 (u8  num);
void log_dec_u16(u16 num);
void log_dec_u32(u32 num);
void log_dec_u64(u64 num);

void log_dec_f32_nan_or_inf(f32 num);
void log_dec_f32_number(f32 num);
void log_dec_f32(f32 num);

#define log_dec_num(num)     \
  _Generic((num),            \
           s8:  log_dec_s8,  \
           s16: log_dec_s16, \
           s32: log_dec_s32, \
           s64: log_dec_s64, \
           u8:  log_dec_u8,  \
           u16: log_dec_u16, \
           u32: log_dec_u32, \
           u64: log_dec_u64, \
           f32: log_dec_f32) \
          (num)

// Hexadecimal
#define U8_MAX_HEX_STR_SIZE   2
#define U16_MAX_HEX_STR_SIZE  4
#define U32_MAX_HEX_STR_SIZE  8
#define U64_MAX_HEX_STR_SIZE 16
#define S8_MAX_HEX_STR_SIZE   2
#define S16_MAX_HEX_STR_SIZE  4
#define S32_MAX_HEX_STR_SIZE  8
#define S64_MAX_HEX_STR_SIZE 16
#define F32_MAX_HEX_STR_SIZE  8

void log_sized_hex_s8 (s8  num, u64 nibble_to_write_count);
void log_sized_hex_s16(s16 num, u64 nibble_to_write_count);
void log_sized_hex_s32(s32 num, u64 nibble_to_write_count);
void log_sized_hex_s64(s64 num, u64 nibble_to_write_count);
void log_sized_hex_u8 (u8  num, u64 nibble_to_write_count);
void log_sized_hex_u16(u16 num, u64 nibble_to_write_count);
void log_sized_hex_u32(u32 num, u64 nibble_to_write_count);
void log_sized_hex_u64(u64 num, u64 nibble_to_write_count);
void log_sized_hex_f32(f32 num, u64 nibble_to_write_count);

#define log_sized_hex_num(num, nibble_to_write_count) \
  _Generic((num),                                     \
           s8:  log_sized_hex_s8,                     \
           s16: log_sized_hex_s16,                    \
           s32: log_sized_hex_s32,                    \
           s64: log_sized_hex_s64,                    \
           u8:  log_sized_hex_u8,                     \
           u16: log_sized_hex_u16,                    \
           u32: log_sized_hex_u32,                    \
           u64: log_sized_hex_u64,                    \
           f32: log_sized_hex_f32)                    \
          (num, nibble_to_write_count)

void log_hex_s8 (s8  num);
void log_hex_s16(s16 num);
void log_hex_s32(s32 num);
void log_hex_s64(s64 num);
void log_hex_u8 (u8  num);
void log_hex_u16(u16 num);
void log_hex_u32(u32 num);
void log_hex_u64(u64 num);
void log_hex_f32(f32 num);

#define log_hex_num(num)     \
  _Generic((num),            \
           s8:  log_hex_s8,  \
           s16: log_hex_s16, \
           s32: log_hex_s32, \
           s64: log_hex_s64, \
           u8:  log_hex_u8,  \
           u16: log_hex_u16, \
           u32: log_hex_u32, \
           u64: log_hex_u64, \
           f32: log_hex_f32) \
          (num)


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Non-alphanumeric types logging
void    log_bool(u64 boolean);
#define log_pointer(ptr) log_sized_hex_u64((u64)(ptr), 16)


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Compounds logging
// Both must be greater or equal to 1 and match the other's value: BYTE_COUNT_FRAC_DIV should be
// 1 followed by BYTE_COUNT_FRAC_SIZE zeros
#define BYTE_COUNT_FRAC_SIZE 2
#define BYTE_COUNT_FRAC_DIV  100

// Log the passed count of bytes shortened to be human-readable (if necessary), followed by a space
// character (' '), its matching decimal unit prefix (if any), and the unit character ('B').
//
// The shortened human-readable version of the byte count has BYTE_COUNT_FRAC_SIZE fractional digits
// if it is superior to 1000. The least significant fractional digit is not rounded.
// For instance:
// - 500     will be displayed as "500 B"
// - 5000000 will be displayed as "5.00 MB"
// - 5000001 will be displayed as "5.00 MB"
// - 5009999 will be displayed as "5.00 MB"
// - 5010000 will be displayed as "5.01 MB"
// - 5999999 will be displayed as "5.99 MB"
//
// The decimal unit prefix will be one of:
// - 'K' for Kilo (x 1000^1)
// - 'M' for Mega (x 1000^2)
// - 'G' for Giga (x 1000^3)
// - 'T' for Tera (x 1000^4)
// - 'P' for Peta (x 1000^5)
// - 'E' for Exa  (x 1000^6)
void log_byte_count_dec_unit(u64 byte_count);


// Log the passed count of bytes shortened to be human-readable (if necessary), followed by a space
// character (' '), its matching binary unit prefix (if any), and the unit character ('B').
//
// The shortened human-redable version of the byte count has BYTE_COUNT_FRAC_SIZE fractional digits
// if it is superior to 1024 (2^10). The least significant fractional digit is not rounded.
// For instance:
// - 500                                will be displayed as "500 B"
// - 5242880 = 5 x 2^20                 will be displayed as "5.00 MiB"
// - 5242881 = 5 x 2^20 + 1             will be displayed as "5.00 MiB"
// - 5253365 = 5 x 2^20 + 10 x 2^10     will be displayed as "5.00 MiB" (integer division truncates)
// - 5253366 = 5 x 2^20 + 10 x 2^10 + 1 will be displayed as "5.01 MiB"
// - 6291455 = 6 x 2^20 - 1             will be displayed as "5.99 MiB"
//
// The binary unit prefix will be one of:
// - "Ki" for Kibi (x 1024^1)
// - "Mi" for Mebi (x 1024^2)
// - "Gi" for Gibi (x 1024^3)
// - "Ti" for Tebi (x 1024^4)
// - "Pi" for Pebi (x 1024^5)
// - "Ei" for Exbi (x 1024^6)
void log_byte_count_bin_unit(u64 byte_count);


// Log the passed Windows or Linux API error code in decimal and its matching description as:
//   "<OS> API error <decimal error code>: <error description>"
void log_os_api_error(u32 error_code);


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//// Utilities

// Get the count of leading zeros in num
u64 lzcnt32(u32 num);

// Get the count of leading zeros in num
u64 lzcnt64(u64 num);

// From low to high bits, everytime a 1 bit is encounted in mask, deposit the bit located at the
// same index in 'a' into the destination (from low to high bits as well)
u32 pdep32(u32 a, u32 mask);

// Reverse the order of a
u32 bswap32(u32 a);

// Get the most significant 1 bit index of num
u64 get_msb_1_bit_idx_u32(u32 num);

// Get the most significant 1 bit index of num
u64 get_msb_1_bit_idx_u64(u64 num);

// Get the count of base-10 digit in num
u64 u32_digit_count(u32 num);

// Get the count of base-10 digit in num
u64 u64_digit_count(u64 num);

// Get the count of base-2 bit in num
u64 u32_bit_count(u32 num);

// Get the count of base-2 bit in num
u64 u64_bit_count(u64 num);

// Get the count of base-16 nibble in num
u64 u32_nibble_count(u32 num);

// Get the count of base-16 nibble in num
u64 u64_nibble_count(u64 num);

// Returns whether f32 is a number (1) of not (0). A f32 is not a number if it +/-qnan, +/-snan, or
// +/- infinity. Otherwise, it is a number
u64 f32_is_a_number(f32 num);

// Returns in "unicode" the Unicode number of the next UTF-16 code-point pointed by "utf16". The
// count of u16 read is returned
u64 utf16_code_point_to_unicode(const u16* utf16, u32* unicode);

// Returns in "utf8" the UTF-8 encoding of Unicode number passed in "unicode". The count of u8
// that were written to is returned
u64 unicode_to_utf8_code_point(u32 unicode, u8* utf8);

#else // defined(LOGS_ENABLED) && (LOGS_ENABLED != 0)
#  pragma message("Logs disabled. Define LOGS_ENABLED or set LOGS_ENABLED to a non-zero value to " \
                  "enable logs")
#  define logs_open_console_output()                               do { } while (0)
#  define logs_close_console_output()                              do { } while (0)
#  define logs_enable_console_ansi_escape_sequence()               do { } while (0)
#  define logs_disable_console_ansi_escape_sequence()              do { } while (0)
#  define logs_open_file_output(file_path)                         do { (void)(file_path); } while (0)
#  define logs_close_file_output()                                 do { } while (0)
#  define logs_flush_to(output)                                    do { (void)output; } while (0)
#  define logs_flush()                                             do { } while (0)
#  define logs_buffer_remaining_bytes()                            0
#  define log_ascii_char(char_character)                           do { (void)(char_character); } while (0)
#  define log_utf8_character(character)                            do { (void)(character); } while (0)
#  define log_utf16_character(ucharacter)                          do { (void)(character); } while (0)
#  define log_character(character)                                 do { (void)(character); } while (0)
#  define log_sized_ascii_str(char_character)                      do { (void)(char_character); } while (0)
#  define log_sized_utf8_str(str, char_count)                      do { (void)(str); (void)(char_count); } while (0)
#  define log_sized_utf16_str(str, char16_count)                   do { (void)(str); (void)(char16_count); } while (0)
#  define log_str(str, count)                                      do { (void)(str); (void)(count); } while (0)
#  define log_null_terminated_ascii_str(char_character)            do { (void)(char_character); } while (0)
#  define log_null_terminated_utf8_str(str)                        do { (void)(str); } while (0)
#  define log_null_terminated_utf16_str(str)                       do { (void)(str); } while (0)
#  define log_null_terminated_str(str)                             do { (void)(str); } while (0)
#  define log_literal_str(str)                                     do { (void)(str); } while (0)
#  define log_sized_bin_s8(num, bit_to_write_count)                do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_s16(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_s32(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_s64(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_u8(num, bit_to_write_count)                do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_u16(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_u32(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_u64(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_f32(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_num(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_bin_s8(num)                                          do { (void)(num); } while (0)
#  define log_bin_s16(num)                                         do { (void)(num); } while (0)
#  define log_bin_s32(num)                                         do { (void)(num); } while (0)
#  define log_bin_s64(num)                                         do { (void)(num); } while (0)
#  define log_bin_u8(num)                                          do { (void)(num); } while (0)
#  define log_bin_u16(num)                                         do { (void)(num); } while (0)
#  define log_bin_u32(num)                                         do { (void)(num); } while (0)
#  define log_bin_u64(num)                                         do { (void)(num); } while (0)
#  define log_bin_f32(num)                                         do { (void)(num); } while (0)
#  define log_bin_num(num)                                         do { (void)(num); } while (0)
#  define log_sized_dec_u64(num, digit_to_write_count)             do { (void)(num); (void)(digit_to_write_count); } while (0)
#  define log_sized_dec_s64(num, digit_to_write_count)             do { (void)(num); (void)(digit_to_write_count); } while (0)
#  define log_sized_dec_f32_number(num, frac_digit_to_write_count) do { (void)(num); (void)(frac_digit_to_write_count); } while (0)
#  define log_sized_dec_f32(num, frac_digit_to_write_count)        do { (void)(num); (void)(frac_digit_to_write_count); } while (0)
#  define log_sized_dec_s8(num, digit_to_write_count)              do { (void)(num); (void)(digit_to_write_count); } while (0)
#  define log_sized_dec_s16(num, digit_to_write_count)             do { (void)(num); (void)(digit_to_write_count); } while (0)
#  define log_sized_dec_s32(num, digit_to_write_count)             do { (void)(num); (void)(digit_to_write_count); } while (0)
#  define log_sized_dec_u8(num, digit_to_write_count)              do { (void)(num); (void)(digit_to_write_count); } while (0)
#  define log_sized_dec_u16(num, digit_to_write_count)             do { (void)(num); (void)(digit_to_write_count); } while (0)
#  define log_sized_dec_u32(num, digit_to_write_count)             do { (void)(num); (void)(digit_to_write_count); } while (0)
#  define log_sized_dec_num(num, digit_to_write_count)             do { (void)(num); (void)(digit_to_write_count); } while (0)
#  define log_dec_f32_nan_or_inf(num)                              do { (void)(num); } while (0)
#  define log_dec_f32_number(num)                                  do { (void)(num); } while (0)
#  define log_dec_f32(num)                                         do { (void)(num); } while (0)
#  define log_dec_s8(num)                                          do { (void)(num); } while (0)
#  define log_dec_s16(num)                                         do { (void)(num); } while (0)
#  define log_dec_s32(num)                                         do { (void)(num); } while (0)
#  define log_dec_s64(num)                                         do { (void)(num); } while (0)
#  define log_dec_u8(num)                                          do { (void)(num); } while (0)
#  define log_dec_u16(num)                                         do { (void)(num); } while (0)
#  define log_dec_u32(num)                                         do { (void)(num); } while (0)
#  define log_dec_u64(num)                                         do { (void)(num); } while (0)
#  define log_dec_num(num)                                         do { (void)(num); } while (0)
#  define log_sized_hex_u64(num, nibble_to_write_count)            do { (void)(num); (void)(nibble_to_write_count); } while (0)
#  define log_sized_hex_s8(num, nibble_to_write_count)             do { (void)(num); (void)(nibble_to_write_count); } while (0)
#  define log_sized_hex_s16(num, nibble_to_write_count)            do { (void)(num); (void)(nibble_to_write_count); } while (0)
#  define log_sized_hex_s32(num, nibble_to_write_count)            do { (void)(num); (void)(nibble_to_write_count); } while (0)
#  define log_sized_hex_s64(num, nibble_to_write_count)            do { (void)(num); (void)(nibble_to_write_count); } while (0)
#  define log_sized_hex_u8(num, nibble_to_write_count)             do { (void)(num); (void)(nibble_to_write_count); } while (0)
#  define log_sized_hex_u16(num, nibble_to_write_count)            do { (void)(num); (void)(nibble_to_write_count); } while (0)
#  define log_sized_hex_u32(num, nibble_to_write_count)            do { (void)(num); (void)(nibble_to_write_count); } while (0)
#  define log_sized_hex_f32(num, nibble_to_write_count)            do { (void)(num); (void)(nibble_to_write_count); } while (0)
#  define log_sized_hex_num(num, nibble_to_write_count)            do { (void)(num); (void)(nibble_to_write_count); } while (0)
#  define log_hex_s8(num)                                          do { (void)(num); } while (0)
#  define log_hex_s16(num)                                         do { (void)(num); } while (0)
#  define log_hex_s32(num)                                         do { (void)(num); } while (0)
#  define log_hex_s64(num)                                         do { (void)(num); } while (0)
#  define log_hex_u8(num)                                          do { (void)(num); } while (0)
#  define log_hex_u16(num)                                         do { (void)(num); } while (0)
#  define log_hex_u32(num)                                         do { (void)(num); } while (0)
#  define log_hex_u64(num)                                         do { (void)(num); } while (0)
#  define log_hex_f32(num)                                         do { (void)(num); } while (0)
#  define log_hex_num(num)                                         do { (void)(num); } while (0)
#  define log_bool(boolean)                                        do { (void)(boolean); } while (0)
#  define log_pointer(ptr)                                         do { (void)(ptr); } while (0)
#  define log_byte_count_dec_unit(byte_count)                      do { (void)(byte_count); } while (0)
#  define log_byte_count_bin_unit(byte_count)                      do { (void)(byte_count); } while (0)
#  define log_os_api_error(error_code)                             do { (void)(error_code); } while (0)
#  define lzcnt32(num)                                             do { (void)(num); } while (0)
#  define lzcnt64(num)                                             do { (void)(num); } while (0)
#  define pdep32(a, mask)                                          do { (void)(a); (void)(mask); } while (0)
#  define bswap32(a)                                               do { (void)(a); } while (0)
#  define get_msb_1_bit_idx_u32(num)                               do { (void)(num); } while (0)
#  define get_msb_1_bit_idx_u64(num)                               do { (void)(num); } while (0)
#  define u32_digit_count(num)                                     do { (void)(num); } while (0)
#  define u64_digit_count(num)                                     do { (void)(num); } while (0)
#  define u32_bit_count(num)                                       do { (void)(num); } while (0)
#  define u64_bit_count(num)                                       do { (void)(num); } while (0)
#  define u32_nibble_count(num)                                    do { (void)(num); } while (0)
#  define u64_nibble_count(num)                                    do { (void)(num); } while (0)
#  define f32_is_a_number(num)                                     do { (void)(num); } while (0)
#  define utf16_code_point_to_unicode(utf16, unicode)              do { (void)(utf16); (void)(unicode); } while (0)
#  define unicode_to_utf8_code_point(unicode, utf8)                do { (void)(unicode); (void)(utf8); } while (0)
#endif // defined(LOGS_ENABLED) && (LOGS_ENABLED != 0)
