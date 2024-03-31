# Logs
Personal C buffered logging library. Example (available in [`example.c`](example.c)):
```C
#include "logs.h"

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
```

You can compile this example yourself by running [`build.bat`](build.bat) (requires Visual Studio
Build Tools or the Native Desktop workload).

## Features
- For Windows XP and above
- Support for x86 64-bit and 32-bit architectures
- Compiles with MSVC (GCC and Clang not tested)
- No C runtime, no C standard library
- Output steams control (open, close, disable, enable, flush, ANSI escape sequences toggling)
  - Provides 1 console output, with ANSI escape sequences, which is either created or reused
  - Provides 1 named file output, which is either created or reused
- Custom formatting of some basic types:
  - Signed and unsigned integers up to 64-bit, in decimal, hexadecimal and binary format
  - Floating-point values up to -/+ 2,147,483,648 with 6 fractional digits.
    NaN and infinities are represented as -/+ 2,147,483,648, depending on the sign bit
- Buffered logging of the above types, literal constant strings, C strings, and single characters
- Logs are turned off by default to prevent any accidental performance hit, and are enabled by
  defining the compile-time macro `ENABLE_LOGS` (setting it to 0 disables logs)

The code in this repository is inspired from
[Christopher "skeeto" Wellons](https://github.com/skeeto)'s excellent article
"[Let's implement buffered, formatted output](https://nullprogram.com/blog/2023/02/13/)"


## Repository files
- [`logs.h`](logs.h) / [`logs.c`](logs.c): logs output management, logs appending functions
- [`num_to_str.h`](num_to_str.h) / [`num_to_str.c`](num_to_str.c): number to string formatting
  functions
- [`types.h`](types.h): custom typedefs, for personal convience
- [`example.c`](example.c): simple example usage of this library
- [`build.bat`](build.bat): sample script to compile the example provided, and to show how to
  include and compile this library into another codebase


## Rational
### Why support Windows only?
Most computers games are targeted at Windows. Nowadays,
[Proton](https://github.com/ValveSoftware/Proton) handles running Windows games on Linux very well,
so Windows is my first choice as well. It is also the OS I work with. Nonetheless, I'm interested
in adding some cross-OS/ISA/platform support.

### `s32`? `u64`?
See [`types.h`](types.h).  
`s` is for `signed`.  
`u` is for `unsigned`.  
`f` is for "floating-point number".  
The number following is the type's width in bits.  
This is purely for convenience.

### Why not use printf, or the C runtime and standard library?
- I like experimenting and understanding what it takes to build even the most mundane things
- I'm only using a small subset of features of the printf's family of functions in my day-to-day work
- I can manage how logs are buffered, with my own straightforward alternative
- Compiling this library ([`example.c`](example.c) included) creates an 5.6 KB executable. By
  including the C runtime, it grows to 115 KB. I appreciate eliminating that big of a dependency:
  in theory, the program could fit in the L1 cache of a 20+-years-old CPU

### Why is there no `double`/`f64`-to-string formatting function?
I almost never use `double`. If I ever need it, then I'll implement something.


## Possible improvements
- Support Unicode characters for logs and log file names
- Add a variadic, printf-like log function or macro (e.g. `logs_append("%s is %u", "Albert", 23)`)
- Implement a macro to stringify compile-time constants passed to log functions
  (e.g. `logs_append_float(123.456f)` would compile to an equivalent of `logs_append_literal(str)`)
- Prevent ANSI escape sequences from appearing in the log file (might require changing code
  architecture)
- Would mapping the log file to the process' virtual memory and using a view to edit it be better
  than writing to it with `WriteFile`? See
  [`CreateFileMapping`](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga)
  and
  [`MapViewOfFile`](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile)
  - Alternatively, disable file caching for the file output (see
  [`FILE_FLAG_NO_BUFFERING`](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea#FILE_FLAG_NO_BUFFERING),
  [`FILE_FLAG_WRITE_THROUGH`](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea#FILE_FLAG_WRITE_THROUGH`)
  and
  [Caching behavior](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea#caching-behavior))
- Add some native support for other OS (Linux?) and other CPU architectures (ARM?)


## License
The code in this repository is released in the public domain. You can use it with no constraint.
