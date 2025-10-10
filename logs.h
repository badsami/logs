#pragma once
#include "types.h"
#include "to_str_utilities.h"

// /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\
// None of the functions declared here perform checks to ensure there is enough space in the log
// buffer. You are in charge of picking a log buffer size that is appropriate to your use-case, and
// of calling logs_flush() after appending your content to the log buffer


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Data
#if !defined(LOGS_BUFFER_SIZE) || (LOGS_BUFFER_SIZE == 0)
// Default size of standard I/O buffers
#  define LOGS_BUFFER_SIZE 4096
#endif

// Index of available outputs in logs.outputs
enum logs_output_idx
{
  LOGS_CONSOLE_OUTPUT = 0,
  LOGS_FILE_OUTPUT    = 1,
  
  LOGS_OUTPUT_COUNT
};
typedef enum logs_output_idx logs_output_idx;

struct logs
{
  // Characters storage, encoded as UTF-8 or ASCII
  u8 buffer[LOGS_BUFFER_SIZE];

  // Output handles
  void* outputs[LOGS_OUTPUT_COUNT];

  // Index past the last character of the buffer
  u32 buffer_end_idx;

  // Meant to be used with logs_output_idx. 1 bit represents 1 output.
  // If the output's bit is 1, it is enabled, otherwise it is disabled
  u32 outputs_state_bits;

  // If the console used to output logs is borrowed, restore its original output code page
  // when logs_close_console_output() is called
  u32 console_original_output_code_page;
};

typedef u16 WCHAR; // same as Windows


#if defined(LOGS_ENABLED) && (LOGS_ENABLED != 0) 
extern struct logs logs;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// Output management

// Open the console output, where flushed logs will be written
void logs_open_console_output(void);

// Close the console, which will no longer receive logs
void logs_close_console_output(void);

// Open a file to append the logs to. file_path is an ASCII-encoded relative or absolute path ("").
// If the file exists, logs will be appended. If it doesn't, the file is created.
void logs_open_file_output_ascii(const char* file_path);

// Open a file to append the logs to. file_path is UTF16-encoded relative or absolute path
// (u"" or L""). If the file exists, logs will be appended. If it doesn't, the file is created.
void logs_open_file_output_utf16(const WCHAR* file_path);

// Windows supports ASCII and UTF-16 file names, but not UTF-8
#define logs_open_file_output(file_path)              \
  _Generic((file_path),                               \
           char*:        logs_open_file_output_ascii, \
           const char*:  logs_open_file_output_ascii, \
           WCHAR*:       logs_open_file_output_utf16, \
           const WCHAR*: logs_open_file_output_utf16) \
          (file_path)

// Close the log file, which will no longer receive logs
void logs_close_file_output(void);

// Stop writing logs to this output, but keep it open
void logs_disable_output(logs_output_idx output_idx);

// Start writing logs to the output again (outputs, once open, are enabled by default)
void logs_enable_output(logs_output_idx output_idx);

// Write the content of the log buffer to enabled outputs, and set the log buffer end index to 0
void logs_flush(void);


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Memory
// Get the number of remaining available space in logs.buffer, in bytes
u32 logs_buffer_remaining_bytes(void);


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Characters & strings logging
// Append a single ASCII character ('a') to the log buffer.
// ASCII characters are encoded the same way as UTF-8
#define log_ascii_char(char_character) log_utf8_character(char_character)

// Append a single UTF-8 code point ('\xC3') to the log buffer
void log_utf8_character(char character);

// Append a single UTF-16 unit ('\u732B') to the log buffer
void log_utf16_character(WCHAR character);

// In C, an ASCII or UTF-8 character literal is an int by default
#define log_character(character)       \
  _Generic((character),                \
           u8:    log_utf8_character,  \
           char:  log_utf8_character,  \
           int:   log_utf8_character,  \
           WCHAR: log_utf16_character) \
          (character)

// Append a chain of ASCII-encoded characters ("abc") of predetermined to the log buffer.
// ASCII characters are encoded the same way as UTF-8
#define log_sized_ascii_str(str, char_count) log_sized_utf8_str(str, char_count)

// Append a chain of UTF-8-encoded characters (u8"Fluß") of predetermined size to the log buffer
void log_sized_utf8_str(const char* str, u32 char_count);

// Append a chain of UTF-16-encoded characters (u"çéÖ", L"çéÖ") of predetermined size to the
// log buffer
void log_sized_utf16_str(const WCHAR* str, u32 wchar_count);

#define log_sized_str(str, count)             \
  _Generic((str),                             \
           char*:        log_sized_utf8_str,  \
           const char*:  log_sized_utf8_str,  \
           u8*:          log_sized_utf8_str,  \
           const u8*:    log_sized_utf8_str,  \
           WCHAR*:       log_sized_utf16_str, \
           const WCHAR*: log_sized_utf16_str) \
          (str, count)

// Append a null-termianted chain of ASCII-encoded characters ("abc") to the log buffer.
// ASCII characters are encoded the same way as UTF-8
#define log_null_terminated_ascii_str(char_character) log_null_terminated_utf8_str(char_character)


// Append a null-terminated chain of UTF-8-encoded characters (u8"Fluß") to the log buffer
void log_null_terminated_utf8_str(const char* str);

// Append a null-terminated chain of UTF-16-encoded characters (u"çéÖ", L"çéÖ") to the log
// buffer
void log_null_terminated_utf16_str(const WCHAR* str);

#define log_null_terminated_str(str)                    \
  _Generic((str),                                       \
           char*:        log_null_terminated_utf8_str,  \
           const char*:  log_null_terminated_utf8_str,  \
           WCHAR*:       log_null_terminated_utf16_str, \
           const WCHAR*: log_null_terminated_utf16_str) \
          (str)


// Append a literal string ("abc", u8"Fluß", L"çéÖ") whose size can be determined at
// compile-time to the log buffer, without its final null terminator
#define log_literal_str(str)                  \
  _Generic((str),                             \
           char*:        log_sized_utf8_str,  \
           const char*:  log_sized_utf8_str,  \
           WCHAR*:       log_sized_utf16_str, \
           const WCHAR*: log_sized_utf16_str) \
           (str, (sizeof(str) - sizeof(str[0])) / sizeof(str[0]))


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Binary number logging
void log_sized_bin_s8 (s8  num, u32 bit_to_write_count);
void log_sized_bin_s16(s16 num, u32 bit_to_write_count);
void log_sized_bin_s32(s32 num, u32 bit_to_write_count);
void log_sized_bin_s64(s64 num, u32 bit_to_write_count);
void log_sized_bin_u8 (u8  num, u32 bit_to_write_count);
void log_sized_bin_u16(u16 num, u32 bit_to_write_count);
void log_sized_bin_u32(u32 num, u32 bit_to_write_count);
void log_sized_bin_u64(u64 num, u32 bit_to_write_count);
void log_sized_bin_f32(f32 num, u32 bit_to_write_count);

#define log_sized_bin_num(num, bit_to_write_count) \
  _Generic((num),                                   \
           s8:  log_sized_bin_s8,                   \
           s16: log_sized_bin_s16,                  \
           s32: log_sized_bin_s32,                  \
           s64: log_sized_bin_s64,                  \
           u8:  log_sized_bin_u8,                   \
           u16: log_sized_bin_u16,                  \
           u32: log_sized_bin_u32,                  \
           u64: log_sized_bin_u64,                  \
           f32: log_sized_bin_f32)                  \
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


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Decimal number logging
void log_sized_dec_s8 (s8  num, u32 digit_to_write_count);
void log_sized_dec_s16(s16 num, u32 digit_to_write_count);
void log_sized_dec_s32(s32 num, u32 digit_to_write_count);
void log_sized_dec_s64(s64 num, u32 digit_to_write_count);
void log_sized_dec_u8 (u8  num, u32 digit_to_write_count);
void log_sized_dec_u16(u16 num, u32 digit_to_write_count);
void log_sized_dec_u32(u32 num, u32 digit_to_write_count);
void log_sized_dec_u64(u64 num, u32 digit_to_write_count);

void log_sized_dec_f32_number(f32 num, u32 frac_digit_to_write_count);
void log_sized_dec_f32(f32 num, u32 frac_digit_to_write_count);


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


// The number of digits required to represent a signed integer and its unsigned cast are equal
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


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Hexadecimal number logging
void log_sized_hex_s8 (s8  num, u32 nibble_to_write_count);
void log_sized_hex_s16(s16 num, u32 nibble_to_write_count);
void log_sized_hex_s32(s32 num, u32 nibble_to_write_count);
void log_sized_hex_s64(s64 num, u32 nibble_to_write_count);
void log_sized_hex_u8 (u8  num, u32 nibble_to_write_count);
void log_sized_hex_u16(u16 num, u32 nibble_to_write_count);
void log_sized_hex_u32(u32 num, u32 nibble_to_write_count);
void log_sized_hex_u64(u64 num, u32 nibble_to_write_count);
void log_sized_hex_f32(f32 num, u32 nibble_to_write_count);

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

#else // defined(LOGS_ENABLED) && (LOGS_ENABLED != 0)
#  pragma message("Logs disabled. Define LOGS_ENABLED or set LOGS_ENABLED to a non-zero value to " \
                  "enable logs")
#  define logs_open_console_output()                               do { } while (0)
#  define logs_close_console_output()                              do { } while (0)
#  define logs_enable_console_ansi_escape_sequence()               do { } while (0)
#  define logs_disable_console_ansi_escape_sequence()              do { } while (0)
#  define logs_open_file_output(file_path)                         do { (void)(file_path); } while (0)
#  define logs_close_file_output()                                 do { } while (0)
#  define logs_disable_output(output_idx)                          do { (void)(output_idx); } while (0)
#  define logs_enable_output(output_idx)                           do { (void)(output_idx); } while (0)
#  define logs_flush()                                             do { } while (0)
#  define logs_buffer_available_bytes()                            0u
#  define log_ascii_char(char_character)                           do { (void)(char_character); } while (0)
#  define log_utf8_character(character)                            do { (void)(character); } while (0)
#  define log_utf16_character(ucharacter)                          do { (void)(character); } while (0)
#  define log_character(character)                                 do { (void)(character); } while (0)
#  define log_sized_ascii_str(char_character)                      do { (void)(char_character); } while (0)
#  define log_sized_utf8_str(str, char_count)                      do { (void)(str); (void)(char_count); } while (0)
#  define log_sized_utf16_str(str, wchar_count)                    do { (void)(str); (void)(wchar_count); } while (0)
#  define log_str(str, count)                                      do { (void)(str); (void)(count); } while (0)
#  define log_null_terminated_ascii_str(char_character)            do { (void)(char_character); } while (0)
#  define log_null_terminated_utf8_str(str)                        do { (void)(str); } while (0)
#  define log_null_terminated_utf16_str(str)                       do { (void)(str); } while (0)
#  define log_null_terminated_str(str)                             do { (void)(str); } while (0)
#  define log_literal_str(str)                                     do { (void)(str); } while (0)
#  define log_sized_bin_u64(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_s8(num, bit_to_write_count)                do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_s16(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_s32(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_s64(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_u8(num, bit_to_write_count)                do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_u16(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
#  define log_sized_bin_u32(num, bit_to_write_count)               do { (void)(num); (void)(bit_to_write_count); } while (0)
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
#endif // defined(LOGS_ENABLED) && (LOGS_ENABLED != 0)
