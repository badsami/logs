#if defined(ENABLE_LOGS) && (ENABLE_LOGS != 0)
#pragma once

#include "types.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//// Data
#if !defined(LOGS_BUFFER_SIZE) || (LOGS_BUFFER_SIZE == 0)
#  define LOGS_BUFFER_SIZE 512
#endif

// Index of available outputs in logs.outputs
typedef enum logs_output_idx
{
  LOGS_OUTPUT_CONSOLE = 0,
  LOGS_OUTPUT_FILE,
  LOGS_OUTPUT_COUNT
} logs_output_idx;

struct logs
{
  // Characters storage
  schar8 buffer[LOGS_BUFFER_SIZE];

  // Output handles
  void* outputs[LOGS_OUTPUT_COUNT];

  // Index past the last character of the buffer
  u32 buf_end_idx;

  // Meant to be used with logs_output_idx. 1 bit represents 1 output.
  // If the output's bit is 1, it is enabled, otherwise it is disabled 
  u32 outputs_state_bits;

  // Whether ansi escape sequence parsing is enabled
  u32 using_ansi_esc_seq;
  
  // The original mode the console was in before enabling escape sequences.
  u32 initial_console_mode;
};

extern struct logs logs;


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Output management

// Open the console output, where flushed logs will be written
void logs_open_console_output(void);

// Close the console, which will no longer receive logs
void logs_close_console_output(void);

// Enable ANSI escape sequences interpretation in the console.
// The escape sequences will appear in the log file (except for the escape character itself)
void logs_enable_console_ansi_escape_sequence(void);

// Disable ANSI escape sequences interpretation in the console
void logs_disable_console_ansi_escape_sequence(void);

// Open a file to append the logs to. file_path can be relative or absolute.
// If the file exists, logs will be appended. If it doesn't, the file is created.
void logs_open_file_output(const schar8* file_path);

// Close the log file, which will no longer receive logs
void logs_close_file_output(void);

// Stop writing logs to this output, but keep it open
void logs_disable_output(logs_output_idx output_idx);

// Start writing logs to the output again (outputs, once open, are enabled by default)
void logs_enable_output(logs_output_idx output_idx);

// Write the content of the log buffer to enabled outputs
void logs_flush(void);


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Log formatting and buffering
//// /!\ These functions perform no check to ensure there is enough space in the log buffer /!\

// Append a u8, u16 or u32 to the log buffer
void logs_append_u32(u32 num);

// Append a u64 to the log buffer
void logs_append_u64(u64 num);

// Append a s8, s16 or s32 to the log buffer
void logs_append_s32(s32 num);

// Append a s64 to the log buffer
void logs_append_s64(s64 num);

// Append a floating-point number to the log buffer
void logs_append_f32(f32 num);

// Append the hexadecimal representation of some data to the log buffer
void logs_append_hex(u64 data);

// Append the binary representation of some data to the log buffer
void logs_append_bin(u64 data);

// Append a string to the log buffer
void logs_append_str(const schar8* msg, u32 msg_size);

// Append a C string to the log buffer
void logs_append_cstr(const schar8* msg);

// Append a single character to the log buffer
void logs_append_char(schar8 c);

// Append a literal, compile-time string between quotes to the log buffer
#define logs_append_literal(msg) logs_append_str(msg, sizeof(msg) - 1)

#else // defined(ENABLE_LOGS) && (ENABLE_LOGS != 0)
#  define logs_open_console_output()                  do { } while (0)
#  define logs_close_console_output()                 do { } while (0)
#  define logs_enable_console_ansi_escape_sequence()  do { } while (0)
#  define logs_disable_console_ansi_escape_sequence() do { } while (0)
#  define logs_open_file_output(file_path)            do { } while (0)
#  define logs_close_file_output()                    do { } while (0)
#  define logs_disable_output(output_idx)             do { } while (0)
#  define logs_enable_output(output_idx)              do { } while (0)
#  define logs_flush()                                do { } while (0)
#  define logs_append_u32(num)                        do { } while (0)
#  define logs_append_u64(num)                        do { } while (0)
#  define logs_append_s32(num)                        do { } while (0)
#  define logs_append_s64(num)                        do { } while (0)
#  define logs_append_f32(num)                        do { } while (0)
#  define logs_append_hex(data)                       do { } while (0)
#  define logs_append_bin(data)                       do { } while (0)
#  define logs_append_str(msg, msg_size)              do { } while (0)
#  define logs_append_cstr(msg)                       do { } while (0)
#  define logs_append_char(c)                         do { } while (0)
#  define logs_append_literal(msg)                    do { } while (0)
#endif // defined(ENABLE_LOGS) && (ENABLE_LOGS != 0)
