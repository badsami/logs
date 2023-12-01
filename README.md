# Log
Personal C buffered logging library.
Example usage:
```C
#include "logs.h"
#include "num_to_str.h"

int main()
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
    s32 toggle = i & 0b1;
    logs_enable_output(toggle ? LOGS_OUTPUT_FILE : LOGS_OUTPUT_CONSOLE);
    logs_disable_output(toggle ? LOGS_OUTPUT_CONSOLE : LOGS_OUTPUT_FILE);
    
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

  // Close outputs
  logs_close_file_output();
  logs_close_console_output();
  
  return 0;
}
```

## Features
- For Windows (XP and above)
- No C runtime, no C standard library
- Outputs handling (open, close, disable, enable, flush, ANSI escape sequences toggling)
  - 1 console output, with ANSI escape sequences, created or reused
  - 1 named file output, created or appended to
- Custom formatting of some basic types:
  - Signed and unsigned integers up to 64-bit, in decimal, hexadecimal and binary format
  - Represents floating-point values up to -/+ 2,147,483,648 with 6 fractional digits.
    NaN and infinities are represented as -/+ 2,147,483,648, depending on the sign bit.
- Buffered logging of the above types, literal constant strings, C strings, and single characters
- Logs don't generate any code by default to prevent any accidental performance hit. Add or remove
  any code generated for logs with a compile time macro (`ENABLE_LOGS`)

The code in this repository is inspired from Chris "skeeto" Wellons's excellent article
"[Let's implement buffered, formatted output](https://nullprogram.com/blog/2023/02/13/)"


## Possible improvements
- Support Unicode characters for logs and log file names
- Add a variadic, printf-like log function or macro (e.g. `log_append("%s is %u", "Albert", 23)`)
- Implement a macro to stringify compile-time constants passed to log functions
  (e.g. `log_append_float(123.456f)` would compile to an equivalent of `log_append_literal(str)`)
- Prevent ANSI escape sequences from appearing in the log file (might require changing code architecture)
- Would mapping the log file to the process' virtual memory and using a view to edit it be better
  than writing to it with `WriteFile`? See [`CreateFileMapping`](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga) and
  [`MapViewOfFile`](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile)
  - Alternatively, disable file caching for the file output (see
  [`FILE_FLAG_NO_BUFFERING`](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea#FILE_FLAG_NO_BUFFERING)
  ,
  [FILE_FLAG_WRITE_THROUGH](`https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea#FILE_FLAG_WRITE_THROUGH`)
  and
  [Caching behavior](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea#caching-behavior))
- Add some native support for other OS (Linux?) and other CPU architectures (ARM?)


## Rational
#### Why support Windows only?
Most computers games are targeted at Windows. It is the OS I work with.  
But I'm interested in adding some cross-OS/ISA/platform support.

#### `s32`? `u64`?
See [](types.h)
`s` is for `signed`.  
`u` is for `unsigned`.  
`f` is for "floating-point number".  
The number following is the type's width in bits.  
This is purely for convenience

#### Why not use printf, or the C runtime and standard library?
- I like experimenting and understanding what it takes to build even the most mundane things
- I'm only using a small subset of features of the printf's family of functions
- I can manage buffering the way I want with my own code
- An empty program with just an entry function is ~1.5 KB without the C runtime/standard library, as
  opposed to ~118 KB when enabled. This is madness to me

#### Why is there no `double`/`f64` to string formating function?
I alsmost never use `double`. If I ever need it, then I'll implement something.

## License
You can use the code in this repository however you'd like. Credit is appreciated. 
