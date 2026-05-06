#pragma once

#if defined(__linux__)
#  define LOGS_OS_LINUX
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__NT__) || defined(__CYGWIN__)
#  define LOGS_OS_WINDOWS
#else
#  warning Platform not supported. Things might break
#endif