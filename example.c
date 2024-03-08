// Compilation command lines (inside a native tools command prompt for VS):
//   mkdir build\obj
//   cl.exe /nologo /DWIN32_LEAN_AND_MEAN /DNOMINMAX /DENABLE_LOGS /Fobuild\obj\ /GS- /W4 num_to_str.c logs.c example.c /link /subsystem:console /nodefaultlib /entry:mainCRTStartup /out:build\example.exe kernel32.lib
//   build\example.exe
#include "logs.h"
#include "types.h"

__declspec(dllimport) void __stdcall ExitProcess(u32);

s32 _fltused;

void mainCRTStartup()
{
  const char* logs_file_name = "logs.txt";
  
  // Open outputs
  logs_open_console_output();
  logs_open_file_output(logs_file_name);

  u32 bits = 0x3C000000;
  for (u32 bit_pos = 24; bit_pos > 15; bit_pos--)
  {
    bits |= (1 << bit_pos);
    f32 value = *(f32*)&bits;

    // Format logs
    log_min_hex_u32(bits);
    log_literal(" (");
    log_u32(bits);
    log_literal(") as a f32 is ");
    log_f32(value);
    log_char('\n');

    // When deemed ready, write buffered logs to enabled outputs
    logs_flush();
  }

  // Write to specific outputs
  logs_disable_output(LOGS_OUTPUT_CONSOLE);
  log_literal("========== Logging session end ==========\n\n");
  logs_flush();
  
  logs_enable_output(LOGS_OUTPUT_CONSOLE);
  logs_disable_output(LOGS_OUTPUT_FILE);
  log_literal("\nLogs written to file ");
  log_cstr(logs_file_name);

  // Close outputs, implicitly flushing logs buffer to enabled outputs
  logs_close_file_output();
  logs_close_console_output();

  ExitProcess(0);
}
