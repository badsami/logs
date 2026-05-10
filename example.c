// Compilation command lines:
// - With MSVC on Windows (inside a x64 Native Tools Command Prompt for VS):
//   cl.exe /nologo /DLOGS_ENABLED /std:c11 /utf-8 logs.c example.c /link /entry:mainCRTStartup /nodefaultlib /subsystem:console kernel32.lib
// - With GCC:
//   gcc -DLOGS_ENABLED -fno-builtin -fno-stack-protector -mbmi2 -mlzcnt -nostdlib logs.c example.c
// - With clang:
//   clang -DLOGS_ENABLED -fno-builtin -fno-stack-protector -mbmi2 -mlzcnt -nostdlib logs.c example.c
#include "logs.h"

void run_example(void)
{
  const os_utf_char* const logs_file_name = OS_UTF_STR("Fluß_¼½¾_Öçé_ǅ.txt");
  logs_open_console_output();
  logs_open_file_output(logs_file_name);

  union { u32 bits; f32 val; } num = {.bits = 0x3C000000};
  for (u32 bit_pos = 24; bit_pos > 6; bit_pos--)
  {
    num.bits |= (1 << bit_pos);

    // Format logs
    log_literal_str("0x");
    log_hex_num(num.bits);
    log_literal_str(" (");
    log_dec_num(num.bits);
    log_literal_str(") as a f32 is ");
    log_dec_num(num.val);
    log_character('\n');
  }

  // Then write buffered logs to opened outputs
  logs_flush();

  // Write to specific outputs
  log_literal_str("========== Logging session end ==========\n\n");
  logs_flush_to(LOGS_OUTPUT_FILE);
  
  log_literal_str("\nLogs written to file ");
  log_null_terminated_str(logs_file_name);
  log_character('\n');
  logs_flush_to(LOGS_OUTPUT_CONSOLE);

  // Close outputs
  logs_close_file_output();
  logs_close_console_output();
}


#if defined(LOGS_OS_WINDOWS)
  extern void ExitProcess(u32);
  s32 _fltused;

  void mainCRTStartup(void)
  {
    run_example();
    ExitProcess(0);
  }
  #elif defined(LOGS_OS_LINUX)
  void _start(void)
  {
    run_example();
    __asm__ ("syscall" : : "a"(60), "D"(0));
  }
#endif