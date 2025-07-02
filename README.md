# Logs
C buffered logging library. Format your logs into a buffer, then write them all at once to a console and/or a log file to reduce writes to storage.
Example (available in [`example.c`](example.c)):
```C
#include "logs.h"

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
```

You can compile this example yourself by running [`build.bat`](build.bat), or from a x64 Native Tools Command Prompt for VS by running `cl.exe /DLOGS_ENABLED /std:c11 /utf-8 to_str_utilities.c logs.c example.c /link /subsystem:console /entry:mainCRTStartup /nodefaultlib /out:example.exe kernel32.lib`  
(requires Visual Studio Build Tools or the Native Desktop workload).  

The code in this repository is inspired from
[Christopher "skeeto" Wellons](https://github.com/skeeto)'s excellent article
"[Let's implement buffered, formatted output](https://nullprogram.com/blog/2023/02/13/)"

## Features
- Targets Windows XP and above
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
    - Values outside of [-2^32 + 1, 2^32 - 1] are output as `-big` or `big`, depending on their sign (see rational in [log_dec_f32_number() comments in log.c](https://github.com/badsami/logs/blob/main/logs.c#L512-#L575))
    - `-qnan`, `qnan`, `-snan`, `snan`, `-inf` or `inf` may be output for matching non-number values
    - `-0` will be converted to `0`
    - Without a predetermined size, 6 digits past the period are written to the output. This can be increased up to 9 (see `F32_DEC_FRAC_DEFAULT_STR_SIZE` and `F32_DEC_FRAC_MULT` in [types_max_str_size.h](types_max_str_size.h))
    - When passing a fixed fractional part size in digits that exceeds 9, the fractional part's will be truncated to 9 (see `F32_DEC_FRAC_MAX_STR_SIZE` in [types_max_str_size.h](types_max_str_size.h))
    - Values are written in full. Scientific notation and other notations are not used
  - ASCII, UTF-8 and UTF-16 characters, null-terminated, sized and literal compile-time strings
- Compile-time defined logs buffer size through macro definition `/DLOGS_BUFFER_SIZE`, which defaults to 4 KB
- Helpers to manage logs buffer memory, with [types_max_str_size.h](types_max_str_size.h) to estimate the maximum number of characters a fundamental type may be represented with, and with [logs_buffer_remaining_bytes() in log.c](https://github.com/badsami/logs/blob/main/logs.c#L184-#L190))
- Logs are turned off by default to prevent any accidental performance hit, and are enabled by defining the compile-time macro `LOGS_ENABLED` (setting it to `0` disables logs)

Because this library gives control over the logs buffer size and when it should be written to enabled outputs, all functions, once called, assume there is enough space left in the logs buffer to append the content they are passed. You are in charge of choosing a log buffer size that is appropriate to your needs, and of calling logs_write() before the buffer become over-saturated.  

It is important to compile with [/utf-8](https://learn.microsoft.com/en-us/cpp/build/reference/utf-8-set-source-and-executable-character-sets-to-utf-8?view=msvc-170) if your are passing UTF-8 encoded characters and strings to the functions of this library.


## Repository files
- [`logs.h`](logs.h) / [`logs.c`](logs.c): logs output management, fundamental types formatting and buffering, logs buffer consumption helper
- [`to_str_utilities.h`](to_str_utilities.h) / [`to_str_utilities.c`](to_str_utilities.c): helpers to categorise and efficiently convert numbers to characters
- [`types.h`](types.h): custom typedefs, for convenience
- [`example.c`](example.c): simple example usage of this library
- [`build.bat`](build.bat): sample script to compile the example provided, and to show how to include and compile this library into another codebase


## Rational
### Why support Windows only?
This is the operating system I'm most familiar with, and Wine makes it easy to run code targeted at Windows on Linux.

### `s32`? `u64`?
See [`types.h`](types.h).  
- `s` is for `signed`
- `u` is for `unsigned`
- `f` is for "floating-point number"
- The number following is the type's width in bits
This is purely for convenience.

### Why not use printf, or the C runtime and standard library?
- I like experimenting and understanding what it takes to build even the most simple things
- I'm usually using only a small subset of features from the `printf`'s family of functions
- This small library provides me with better control and understanding performance
- I avoid using the C standard library and Windows C runtime, which often incur hidden performance hits or perform operations I don't need, and increase executable size significantly for small tools/libraries. Using the C runtime, compiling this library ([`example.c`](example.c) included) with `build.bat` results in a 106.5 KB executable. Without the C runtime, the executable shrinks down to 5.5 KB. The former can't fit into the L1 cache of an [Intel's Lion Cove](https://en.wikipedia.org/wiki/Lion_Cove#L0) CPU nor in that of an [AMD's Zen 5](https://en.wikipedia.org/wiki/Zen_5#L1) CPU, both from 2024. The later could fit in the L1 cache of an [Intel's i486](https://en.wikipedia.org/wiki/I486#Differences_between_i386_and_i486) CPU from 1989 or in that of an [AMD's K6](https://en.wikipedia.org/wiki/AMD_K6#Models) CPU from 1997. Isn't that great?

## License
The code in this repository is released in the public domain. You are allowed to use the code in this repository freely.
