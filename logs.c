#if defined(ENABLE_LOGS) && (ENABLE_LOGS != 0)
#include "logs.h"
#include "num_to_str.h"

#define _WIN32_WINNT 0x0501
#include <Windows.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//// Private helpers
static void logs_open_output(const schar8* file_path, u32 access, u32 creation_mode, logs_output_idx output_idx)
{
  logs.outputs[output_idx] = CreateFileA(file_path, access,
                                         FILE_SHARE_WRITE,
                                         0,
                                         creation_mode,
                                         0,
                                         0);
  logs_enable_output(output_idx);
}

static void logs_close_output(logs_output_idx output_idx)
{
  u32 output_mask          = (1 << output_idx);
  u32 output_was_enabled   = logs.outputs_state_bits & output_mask;
  logs.outputs_state_bits &= ~output_mask;
  
  if (logs.buf_end_idx != 0 && output_was_enabled)
  {
    WriteFile(logs.outputs[output_idx], logs.buffer, logs.buf_end_idx, 0, 0);
    if (logs.outputs_state_bits == 0)
    {
      // There are no other enabled outputs, the content of the log buffer is no longer needed
      logs.buf_end_idx = 0;
    }
  }
  
  CloseHandle(logs.outputs[output_idx]);
  logs.outputs[output_idx] = 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Global
struct logs logs =
{
  .outputs            = {0, 0},
  .buf_end_idx        = 0,
  .outputs_state_bits = 0,
  .using_ansi_esc_seq = 0
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Output management
// Console output
void logs_open_console_output(void)
{
  if (logs.outputs[LOGS_OUTPUT_CONSOLE] == 0)
  {
    // If this process already has a console, this will not allocate a new console
    s32 success = AttachConsole(ATTACH_PARENT_PROCESS);
    if (success == 0 && GetLastError() == ERROR_INVALID_HANDLE)
    {
      // This process doesn't have a console. Create a new one
      AllocConsole();
      SetConsoleTitle("Logs");
    }

    logs_open_output("CONOUT$", GENERIC_WRITE, OPEN_EXISTING, LOGS_OUTPUT_CONSOLE);
  }
}

void logs_close_console_output(void)
{
  if (logs.outputs[LOGS_OUTPUT_CONSOLE] != 0)
  {
    logs_close_output(LOGS_OUTPUT_CONSOLE);
    
    // Free the console of this process
    FreeConsole();
  }
}

void logs_enable_console_ansi_escape_sequence(void)
{
  if ((logs.outputs[LOGS_OUTPUT_CONSOLE] != 0) && (logs.using_ansi_esc_seq == 0))
  {
    logs.using_ansi_esc_seq = 1;
    GetConsoleMode(logs.outputs[LOGS_OUTPUT_CONSOLE], (DWORD*)(&logs.initial_console_mode));

    u32 new_console_mode = logs.initial_console_mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                           
    SetConsoleMode(logs.outputs[LOGS_OUTPUT_CONSOLE], new_console_mode);
  }
}

void logs_disable_console_ansi_escape_sequence(void)
{
  if ((logs.outputs[LOGS_OUTPUT_CONSOLE] != 0) && (logs.using_ansi_esc_seq != 0))
  {
    SetConsoleMode(logs.outputs[LOGS_OUTPUT_CONSOLE], logs.initial_console_mode);
    logs.using_ansi_esc_seq = 0;
  }
}

// File output
void logs_open_file_output(const schar8* file_path)
{
  if (logs.outputs[LOGS_OUTPUT_FILE] == 0)
  {
    logs_open_output(file_path, FILE_APPEND_DATA, OPEN_ALWAYS, LOGS_OUTPUT_FILE);
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
void logs_disable_output(logs_output_idx output_idx)
{
  logs.outputs_state_bits &= ~(1 << output_idx);
}

void logs_enable_output(logs_output_idx output_idx)
{
  u32 is_open = (logs.outputs[output_idx] != 0);
  logs.outputs_state_bits |= (is_open << output_idx);
}

void logs_flush(void)
{
  // Trust that the caller knows the log buffer is not empty
  // If an output is enabled, it is open. Parse the output state bits to select destination outputs
  u32 outputs_mask = logs.outputs_state_bits;
  u32 idx          = 0;
  while (outputs_mask != 0)
  {
    if (outputs_mask & 0b1)
    {
      WriteFile(logs.outputs[idx], logs.buffer, logs.buf_end_idx, 0, 0);
    }
    
    idx++;
    outputs_mask >>= 1;
  }

  logs.buf_end_idx = 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Log formatting and buffering
void logs_append_u32(u32 num)
{
  schar8* logs_buf_end = logs.buffer + logs.buf_end_idx;
  u32     num_str_size = u32_to_str(logs_buf_end, num);
  logs.buf_end_idx += num_str_size;
}

void logs_append_u64(u64 num)
{
  schar8* logs_buf_end = logs.buffer + logs.buf_end_idx;
  u32     num_str_size = u64_to_str(logs_buf_end, num);
  logs.buf_end_idx += num_str_size;
}

void logs_append_s32(s32 num)
{
  schar8* logs_buf_end = logs.buffer + logs.buf_end_idx;
  u32     num_str_size = s32_to_str(logs_buf_end, num);
  logs.buf_end_idx += num_str_size;
}

void logs_append_s64(s64 num)
{
  schar8* logs_buf_end = logs.buffer + logs.buf_end_idx;
  u32     num_str_size = s64_to_str(logs_buf_end, num);
  logs.buf_end_idx += num_str_size;
}

void logs_append_f32(f32 num)
{
  schar8* logs_buf_end = logs.buffer + logs.buf_end_idx;
  u32     num_str_size = f32_to_str(logs_buf_end, num);
  logs.buf_end_idx += num_str_size;
}

void logs_append_hex64(u64 data)
{
  schar8* logs_buf_end  = logs.buffer + logs.buf_end_idx;
  u32     data_str_size = hex64_to_str(logs_buf_end, data);
  logs.buf_end_idx += data_str_size;
}

void logs_append_bin64(u64 data)
{
  schar8* logs_buf_end  = logs.buffer + logs.buf_end_idx;
  u32     data_str_size = bin64_to_str(logs_buf_end, data);
  logs.buf_end_idx += data_str_size;
}

void logs_append_str(const schar8* msg, u32 msg_size)
{
  // No checks are performed. Suppose that the log buffer size is adequate and that logs_flush()
  // will be called at the right time.
  const u32 new_buf_end_idx = logs.buf_end_idx + msg_size;
  for (u32 buf_idx = logs.buf_end_idx; buf_idx < new_buf_end_idx; buf_idx++)
  {
    logs.buffer[buf_idx] = *msg++;
  }

  logs.buf_end_idx = new_buf_end_idx;
}

void logs_append_cstr(const schar8* msg)
{
  while (*msg != '\0')
  {
    logs.buffer[logs.buf_end_idx++] = *msg++;
  }
}

void logs_append_char(schar8 c)
{
  // Same as above
  logs.buffer[logs.buf_end_idx] = c;
  logs.buf_end_idx++;
}

#endif // ENABLE_LOGS
