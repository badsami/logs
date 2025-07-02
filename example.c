// Compilation command lines (inside a x64 Native Tools Command Prompt for VS):
// 
//   cl.exe /nologo /DLOGS_ENABLED /std:c11 /utf-8 to_str_utilities.c logs.c example.c /link /subsystem:console /entry:mainCRTStartup /nodefaultlib /out:example.exe kernel32.lib
#include "logs.h"

__declspec(dllimport) void __stdcall ExitProcess(u32);

s32 _fltused;

void mainCRTStartup()
{
  const WCHAR* logs_file_name = u"Fluß_¼½¾_Öçé_ǅ.txt";
  
  // Open outputs
  logs_open_console_output();
  logs_open_file_output(logs_file_name);

  u32 bits = 0x3C000000;
  for (u32 bit_pos = 24; bit_pos > 15; bit_pos--)
  {
    bits |= (1 << bit_pos);
    f32 value = *(f32*)&bits;

    // Format logs
    log_literal_str("0x");
    log_hex_num(bits);
    log_literal_str(" (");
    log_dec_num(bits);
    log_literal_str(") as a f32 is ");
    log_dec_num(value);
    log_character('\n');
  }

  // Then write buffered logs to enabled outputs
  logs_write();

  // Write to specific outputs
  logs_disable_output(LOGS_CONSOLE_OUTPUT);
  log_literal_str("========== Logging session end ==========\n\n");
  logs_write();
  
  logs_enable_output(LOGS_CONSOLE_OUTPUT);
  logs_disable_output(LOGS_FILE_OUTPUT);
  log_literal_str(u8"\nLogs written to file ");
  log_ntstr(logs_file_name);

  // Close outputs, implicitly flushing logs buffer to enabled outputs
  logs_close_file_output();
  logs_close_console_output();

  ExitProcess(0);
}
