#include "logs.h"
#include "num_to_str.h"

__declspec(dllimport) void __stdcall ExitProcess(u32);

s32 _fltused;

void mainCRTStartup()
{
  // Open outputs
  logs_open_console_output();
  logs_open_file_output("logs.txt");
  
  char i_str[2];
  
  for (int i = 0; i < 10; i++)
  {
    // Enable/disable outputs:
    // - when i is odd, write to the logs file only
    // - when i is even, write to logs console only
    logs_enable_output((i & 0b1) ? LOGS_OUTPUT_FILE : LOGS_OUTPUT_CONSOLE);
    logs_disable_output((i & 0b1) ? LOGS_OUTPUT_CONSOLE : LOGS_OUTPUT_FILE);
    
    // Format i to a string once, then reuse the string
    u32 i_str_size = s32_to_str(i_str, i);
    
    for (int j = 0; j < 100; j++)
    {
      // Append content to the logs buffer. Thee size of the logs buffer is
      // limited: it is a compile time macro constant (LOGS_BUFFER_SIZE)
      // defaulting to 512, which can be changed througn compiler flags.
      // No checks are performed to verify if what's appended to the logs
      // buffer actually fits in.
      logs_append_str(i_str, i_str_size);
      logs_append_char('.');
      logs_append_s32(j);
      logs_append_char(' ');
    }

    // Write the buffered logs to open and enabled outputs
    logs_flush();
  }

  logs_append_char('\n');
  logs_flush();

  logs_enable_output(LOGS_OUTPUT_CONSOLE);
  logs_append_literal("\nSee the content of logs.txt in the active directory\n");

  // Close outputs
  logs_close_file_output();
  logs_close_console_output();

  ExitProcess(0);
}
