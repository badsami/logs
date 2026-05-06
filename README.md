# Logs
C buffered logging library for x64 Linux & Windows. Format data into a buffer of pre-determined size on the stack, then write it all at once to selected outputs. No C runtime, no C standard library, no dynamic allocation, minimal system calls and output traffic.
Example (available in [`example.c`](example.c)):
```C
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
    log_literal_str(u8") as a f32 is ");
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
```

You can compile this example yourself:
- On Linux: by running [`build.sh`](build.sh), or by running `cc -DLOGS_ENABLED -fno-builtin -fno-stack-protector -mbmi2 -mlzcnt -nostdlib logs.c example.c` from a terminal
- On Windows: by running [`build.bat`](build.bat), or by running `cl.exe /DLOGS_ENABLED /std:c11 /utf-8 logs.c example.c /link /entry:mainCRTStartup /nodefaultlib /subsystem:console kernel32.lib` from a "x64 Native Tools Command Prompt for VS" (requires Visual Studio Build Tools or the Native Desktop workload)


## Features
- Targets Linux & Windows XP and above
- Supports x86_64
- Compiles with MSVC (GCC and Clang not supported)
- Doesn't use the C runtime, nor the C standard library
- No dynamic allocations, the logs buffer is allocated on the stack
- Offers output streams control (open, close, disable, enable, write)
  - Provides 1 console output which is either created or reused from the calling process, and set to display UTF-8-encoded characters
  - Provides 1 ASCII- or UTF-16-named file output, which is either created or opened, then appended to
- Logging of fundamental types
  - Signed and unsigned integers up to 64-bit, in binary, decimal and hexadecimal format, with or without a pre-determined size in bits, digits or nibbles
  - 32-bit floating point numbers in binary, decimal and hexadecimal format, with or without a pre-determined size in bits or nibbles, or a pre-determined decimal fractional part size, with a few particularities:
    - No rounding is performed on the integer and fractional part
    - Values outside of [-2^32 + 1, 2^32 - 1] are output as `-big` or `big`, depending on their sign (see rational in [log_dec_f32_number() comments in log.c](https://github.com/badsami/logs/blob/main/logs.c#796-829))
    - `-qnan`, `qnan`, `-snan`, `snan`, `-inf` or `inf` may be output for matching non-number values
    - `-0` will be converted to `0`
    - Without a predetermined size, 6 digits past the period are written to the output. This can be increased up to 9 (see `F32_DEC_FRAC_DEFAULT_STR_SIZE` and `F32_DEC_FRAC_MULT` in [logs.h](logs.h))
    - When passing a fixed fractional part size in digits that exceeds 9, the fractional part will be truncated to 9 digits (see `F32_DEC_FRAC_MAX_STR_SIZE` in [logs.h](logs.h))
    - Values are written in full. Scientific notation and other notations are not used
  - Boolean values from 8 to 64 bits
  - Pointers
  - ASCII, UTF-8 and UTF-16 characters, null-terminated, sized and literal compile-time strings
- Logging of miscellaneous compounds of numbers and characters:
  - OS error formatting in a message containing the error's description
  - Count of bytes using decimal or binary unit prefixes, with 2 fractional digits (no rounding is performed)
- Generic function interfaces for function-like macro calls compatible with several types
- Compile-time defined logs buffer size through macro definition `-DLOGS_BUFFER_SIZE`, which defaults to 4 KiB
- Helpers to manage logs buffer memory, through compile-time constants in [logs.h](logs.h) to estimate the maximum number of characters a fundamental type may usen and through [logs_buffer_remaining_bytes()](https://github.com/badsami/logs/blob/main/logs.c#L246-#L252)
- Various exposed utilities functions to call intrinsics, count numerals and perform conversions from UTF-16 to Unicode and from Unicode to UTF-8
- Logs are turned off by default and are enabled by defining the compile-time macro `LOGS_ENABLED` (setting it to `0` disables logs)

> [!NOTE]
> Because this library gives control over the logs buffer size and when it should be written to enabled outputs, all functions, once called, assume there is enough space left in the logs buffer to append the content they are passed. You are in charge of choosing a log buffer size that is appropriate to your needs, and of calling `logs_flush()` before the buffer becomes over-saturated.  


## Rationale
### Why not use printf, or the C runtime and standard library?
- I like experimenting and understanding what it takes to build even the most simple things
- I'm usually using only a small subset of features from the `printf`'s family of functions
- This small library provides me with more explicit control over logs and their outputs
- I avoid using the C standard library and runtime, which is great for executable size
  - Using Windows C runtime, compiling this library (`example.c` included) with `build.bat` results in a 106.5 KiB executable. It cannot fit into the L1 instruction cache of an [Intel's Lion Cove](https://en.wikipedia.org/wiki/Lion_Cove#L0) CPU nor in that of an [AMD's Zen 5](https://en.wikipedia.org/wiki/Zen_5#L1) CPU, both from 2024
  - Without the C runtime, the executable shrinks down to 5120 bytes on Windows and 8712 bytes on Linux, meaning it could fit in the L1 cache of an [Intel's i486](https://en.wikipedia.org/wiki/I486#Differences_between_i386_and_i486) CPU from 1989 or in that of an [AMD's K6](https://en.wikipedia.org/wiki/AMD_K6#Models) CPU from 1997. I find it incredible!


## License
The code in this repository is released in the public domain. You are free to use the code in this repository for any purpose.

I only ask that you do not misrepresent the origin of this code: acknowledging or disclosing its origin is not required, but please do not claim that someone other than me wrote the original software, if inquired.
