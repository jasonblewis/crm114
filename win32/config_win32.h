/* src/config.h.  Generated from config.h.in by configure.  Patched by [i_a] to make it work for MSVC2005.  */
/* src/config.h.in.  Generated from configure.ac by autoheader.  */

/* untrappable ASSERT/VERIFY checks */
#define CRM_ASSERT_IS_UNTRAPPABLE 1

/* DISable ASSERT/VERIFY checks */
/* #undef CRM_DONT_ASSERT */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#undef HAVE_ARPA_INET_H

/* Define to 1 if you have the `bcopy' function. */
#undef HAVE_BCOPY

/* BSD REs */
/* #undef HAVE_BSD_REGEX */

/* Define to 1 if the system has the type `clock_t'. */
#define HAVE_CLOCK_T 1

/* Define to 1 if you have the <crtdbg.h> header file. */
#define HAVE_CRTDBG_H 1

/* Define to 1 if you have the <crt_externs.h> header file. */
/* #undef HAVE_CRT_EXTERNS_H */

/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* Define to 1 if you have the <direct.h> header file. */
#define HAVE_DIRECT_H 1

/* Define to 1 if you have the <dirent.h> header file. */
#undef HAVE_DIRENT_H

/* Define to 1 if you have the `dup2' function. */
#define HAVE_DUP2 1

/* Define to 1 if you have the <endian.h> header file. */
#undef HAVE_ENDIAN_H

/* Define if you have the 'environ' global environment variable */
/* #undef HAVE_ENVIRON */

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the `fabs' function. */
#define HAVE_FABS 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <float.h> header file. */
#define HAVE_FLOAT_H 1

/* Define to 1 if you have the `floor' function. */
#define HAVE_FLOOR 1

/* Define to 1 if you have the `fork' function. */
#undef HAVE_FORK

/* Define to 1 if you have the `getopt' function. */
#undef HAVE_GETOPT

/* Define to 1 if you have the <getopt_ex.h> header file. */
/* #undef HAVE_GETOPT_EX_H */

/* Define to 1 if you have the <getopt.h> header file. */
#undef HAVE_GETOPT_H

/* Define to 1 if you have the `getopt_long' function. */
#undef HAVE_GETOPT_LONG

/* Define to 1 if you have the `getpagesize' function. */
#undef HAVE_GETPAGESIZE

/* Define to 1 if you have the `getpid' function. */
#define HAVE_GETPID 1

/* Define to 1 if you have the `getppid' function. */
#undef HAVE_GETPPID

/* GNU REs */
/* #undef HAVE_GNU_REGEX */

/* Define to 1 if you have the <history.h> header file. */
#undef HAVE_HISTORY_H

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if you have the <io.h> header file. */
#define HAVE_IO_H 1

/* Define to 1 if you have the `isalnum' function. */
#define HAVE_ISALNUM 1

/* Define to 1 if you have the `isalpha' function. */
#define HAVE_ISALPHA 1

/* Define to 1 if you have the `isascii' function. */
#define HAVE_ISASCII 1

/* Define to 1 if you have the `isblank' function. */
#undef HAVE_ISBLANK

/* Define to 1 if you have the `iscntrl' function. */
#define HAVE_ISCNTRL 1

/* Define to 1 if you have the `isdigit' function. */
#define HAVE_ISDIGIT 1

/* Define to 1 if you have the `isgraph' function. */
#define HAVE_ISGRAPH 1

/* Define to 1 if you have the `islower' function. */
#define HAVE_ISLOWER 1

/* Define to 1 if you have the `isprint' function. */
#define HAVE_ISPRINT 1

/* Define to 1 if you have the `ispunct' function. */
#define HAVE_ISPUNCT 1

/* Define to 1 if you have the `isspace' function. */
#define HAVE_ISSPACE 1

/* Define to 1 if you have the `isupper' function. */
#define HAVE_ISUPPER 1

/* Define to 1 if you have the `isxdigit' function. */
#define HAVE_ISXDIGIT 1

/* Define to 1 if you have the <libintl.h> header file. */
#undef HAVE_LIBINTL_H

/* Define to 1 if you have the `m' library (-lm). */
#undef HAVE_LIBM

/* Define if you have a readline compatible library */
#undef HAVE_LIBREADLINE

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <lmcons.h> header file. */
#define HAVE_LMCONS_H 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the `log' function. */
#define HAVE_LOG 1

/* Define to 1 if you have the `log10' function. */
#define HAVE_LOG10 1

/* Define to 1 if you have the `log2' function. */
#undef HAVE_LOG2

/* Define to 1 if you have the `logl' function. */
#define HAVE_LOGL 1

/* compiler understands long long */
#define HAVE_LONG_LONG 1

/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
 * to 0 otherwise. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the <math.h> header file. */
#define HAVE_MATH_H 1

/* Define to 1 if you have the `memchr' function. */
#define HAVE_MEMCHR 1

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have a working `mmap' system call. */
#undef HAVE_MMAP

/* Define to 1 if the system has the type `mode_t'. */
#define HAVE_MODE_T 1

/* Define to 1 if you have the `msync' function. */
#undef HAVE_MSYNC

/* Define to 1 if you have the `munmap' function. */
#undef HAVE_MUNMAP

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
#undef HAVE_NDIR_H

/* Define to 1 if you have the <netinet/in.h> header file. */
#undef HAVE_NETINET_IN_H

/* Define if run-time library offers nanosecond time interval in struct
   stat:c/m/atimensec. */
#undef HAVE_NSEC_STAT_TIMENSEC

/* Define if run-time library offers nanosecond time interval in struct
   stat:c/m/atime_nsec. */
#undef HAVE_NSEC_STAT_TIME_NSEC

/* Define if run-time library offers nanosecond time interval in struct
   stat:c/m/atim.tv_nsec. */
#undef HAVE_NSEC_STAT_TIM_TV_NSEC

/* PCRE REs */
/* #undef HAVE_PCRE_REGEX */

/* Define to 1 if the system has the type `pid_t'. */
#define HAVE_PID_T 1

/* Define to 1 if you have the `pipe' function. */
#undef HAVE_PIPE

/* POSIX REs */
/* #undef HAVE_POSIX_REGEX */

/* Define to 1 if you have the `pow' function. */
#define HAVE_POW 1

/* Define to 1 if you have the <process.h> header file. */
#define HAVE_PROCESS_H 1

/* Define to 1 if you have the <readline.h> header file. */
/* #undef HAVE_READLINE_H */

/* Define if your readline library has \`add_history' */
#undef HAVE_READLINE_HISTORY

/* Define to 1 if you have the <readline/history.h> header file. */
#undef HAVE_READLINE_HISTORY_H

/* Define to 1 if you have the <readline/readline.h> header file. */
#undef HAVE_READLINE_READLINE_H

/* Define to 1 if your system has a GNU libc compatible `realloc' function,
 * and to 0 otherwise. */
#define HAVE_REALLOC 1

/* REs support */
#define HAVE_REGEX 1

/* Define to 1 if you have the <regex.h> header file. */
#define HAVE_REGEX_H 1

/* Define to 1 if you have the `setlocale' function. */
#define HAVE_SETLOCALE 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the `snprintf' function. */
#undef HAVE_SNPRINTF

/* Define to 1 if you have the `sqrt' function. */
#define HAVE_SQRT 1

/* Define to 1 if you have the `stat' function. */
#define HAVE_STAT 1

/* Define to 1 if `stat' has the bug that it succeeds when given the
 * zero-length file name argument. */
/* #undef HAVE_STAT_EMPTY_STRING_BUG */

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if stdbool.h conforms to C99. */
#undef HAVE_STDBOOL_H

/* Define to 1 if you have the <stdint.h> header file. */
#undef HAVE_STDINT_H

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#undef HAVE_STRCASECMP

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strcmp' function. */
#define HAVE_STRCMP 1

/* Define to 1 if you have the `strcspn' function. */
#define HAVE_STRCSPN 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `strftime' function. */
#define HAVE_STRFTIME 1

/* Define to 1 if you have the `stricmp' function. */
#define HAVE_STRICMP 1

/* Define to 1 if cpp supports the ANSI # stringizing operator. */
#define HAVE_STRINGIZE 1

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strmov' function. */
/* #undef HAVE_STRMOV */

/* Define to 1 if you have the `strncasecmp' function. */
#undef HAVE_STRNCASECMP

/* Define to 1 if you have the `strncpy' function. */
#define HAVE_STRNCPY 1

/* Define to 1 if you have the `strnicmp' function. */
#define HAVE_STRNICMP 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if the system has the type `struct stat'. */
#define HAVE_STRUCT_STAT 1

/* Define to 1 if you have the `sysconf' function. */
#undef HAVE_SYSCONF

/* Define to 1 if you have the `system' function. */
#define HAVE_SYSTEM 1

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
 */
#undef HAVE_SYS_DIR_H

/* Define to 1 if you have the <sys/mman.h> header file. */
#undef HAVE_SYS_MMAN_H

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
 */
#undef HAVE_SYS_NDIR_H

/* Define to 1 if you have the <sys/param.h> header file. */
#undef HAVE_SYS_PARAM_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/times.h> header file. */
#undef HAVE_SYS_TIMES_H

/* Define to 1 if you have the <sys/time.h> header file. */
#undef HAVE_SYS_TIME_H

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/utime.h> header file. */
#define HAVE_SYS_UTIME_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#undef HAVE_SYS_WAIT_H

/* Define to 1 if you have the `times' function. */
#undef HAVE_TIMES

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* TRE REs */
#define HAVE_TRE_REGEX 1

/* Define to 1 if you have the <tre/regex.h> header file. */
#undef HAVE_TRE_REGEX_H

/* Define to 1 if you have the `truncate' function. */
#undef HAVE_TRUNCATE

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define to 1 if you have the `utime' function. */
#undef HAVE_UTIME

/* Define to 1 if you have the <utime.h> header file. */
#undef HAVE_UTIME_H

/* SYSV 8 REs */
/* #undef HAVE_V8_REGEX */

/* SYSV 8 RE exports regsub */
/* #undef HAVE_V8_REGSUB */

/* Define to 1 if you have the <varargs.h> header file. */
/* #undef HAVE_VARARGS_H */

/* Define to 1 if you have the `vfork' function. */
#undef HAVE_VFORK

/* Define to 1 if you have the <vfork.h> header file. */
#undef HAVE_VFORK_H

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the `waitpid' function. */
#undef HAVE_WAITPID

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to 1 if `fork' works. */
#undef HAVE_WORKING_FORK

/* Define to 1 if `vfork' works. */
#undef HAVE_WORKING_VFORK

/* Define to 1 if the system has the type `_Bool'. */
#define HAVE__BOOL 1

/* Define to 1 if you have the `_set_output_format' function. */
#define HAVE__SET_OUTPUT_FORMAT 1

/* Define to 1 if you have the `_snprintf' function. */
#define HAVE__SNPRINTF 1

/* Define to 1 if you have the `_stat' function. */
#define HAVE__STAT 1

/* Define to 1 if you have the `_utime' function. */
#define HAVE__UTIME 1

/* Define to 1 if you have the `_vsnprintf' function. */
#define HAVE__VSNPRINTF 1

/* Define to 1 if you have the `__debugbreak' function. */
#define HAVE___DEBUGBREAK 1

/* Define if you have the '__environ' global environment variable */
#undef HAVE___ENVIRON

/* Define if compiler implements __FUNCTION__. */
#define HAVE___FUNCTION__ 1

/* Define if compiler implements __func__. */
#undef HAVE___FUNC__

/* Set host type */
#define HOSTTYPE "windows-MS"

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
 * slash. */
#undef LSTAT_FOLLOWS_SLASHED_SYMLINK

/* Define to 1 if your processor stores words with the most significant byte
 * first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef MACHINE_IS_BIG_ENDIAN */

/* Define to 1 if your processor stores words with the least significant byte
 * first (like Intel and VAX). */
#define MACHINE_IS_LITTLE_ENDIAN 1

/* Name of package */
#define PACKAGE "crm114"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "crm114-general@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "CRM114"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "CRM114 20070815-BlameBadData"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "crm114"

/* Define to the version of this package. */
#define PACKAGE_VERSION "20070815-BlameBadData"

/* "enable replacement memmove if system memmove is broken or missing" */
/* #undef PREFER_PORTABLE_MEMMOVE */

/* "enable replacement (v)snprintf if system (v)snprintf is broken" */
/* #undef PREFER_PORTABLE_SNPRINTF */

/* Define to 1 if the C compiler supports function prototypes. */
#define PROTOTYPES 1

/* Define to 1 if the `setvbuf' function takes the buffering type as its
 * second argument and the buffer pointer as the third, as on System V before
 * release 3. */
/* #undef SETVBUF_REVERSED */

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long int', as computed by sizeof. */
#define SIZEOF_LONG_INT 4

/* The size of `long long int', as computed by sizeof. */
#define SIZEOF_LONG_LONG_INT 8

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
/* #undef STAT_MACROS_BROKEN */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* distribution archive filename postfix code of the software */
#define TAR_FILENAME_POSTFIX "Ger-190"

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#undef TIME_WITH_SYS_TIME

/* Version number of package */
#define VERSION "20070815-BlameBadData"

/* version suffix code of the software */
#define VER_SUFFIX ""

/* Define to 1 if on AIX 3.
 * System headers sometimes define this.
 * We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* # undef _ALL_SOURCE */
#endif

/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
#undef _GNU_SOURCE
#endif

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
 * this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Define for Solaris 2.5.1 so the uint32_t typedef from <sys/synch.h>,
 * <pthread.h>, or <semaphore.h> is not used. If the typedef was allowed, the
 #define below would cause a syntax error. */
/* #undef _UINT32_T */

/* Define for Solaris 2.5.1 so the uint64_t typedef from <sys/synch.h>,
 * <pthread.h>, or <semaphore.h> is not used. If the typedef was allowed, the
 #define below would cause a syntax error. */
/* #undef _UINT64_T */

/* Define for Solaris 2.5.1 so the uint8_t typedef from <sys/synch.h>,
 * <pthread.h>, or <semaphore.h> is not used. If the typedef was allowed, the
 #define below would cause a syntax error. */
/* #undef _UINT8_T */

/* Define to 1 if type `char' is unsigned and you are not using gcc.  */
#ifndef __CHAR_UNSIGNED__
/* # undef __CHAR_UNSIGNED__ */
#endif

/* Define like PROTOTYPES; this can be used by system headers. */
#define __PROTOTYPES 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
 * calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#define inline __inline
#endif

typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8 int8_t;

typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;

/* Define to the type of a signed integer type of width exactly 16 bits if
 * such a type exists and the standard includes do not define it. */
/* #undef int16_t */

/* Define to the type of a signed integer type of width exactly 32 bits if
 * such a type exists and the standard includes do not define it. */
/* #undef int32_t */

/* Define to the type of a signed integer type of width exactly 64 bits if
 * such a type exists and the standard includes do not define it. */
/* #undef int64_t */

/* Define to the type of a signed integer type of width exactly 8 bits if such
 * a type exists and the standard includes do not define it. */
/* #undef int8_t */

/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to `int' if <sys/types.h> does not define. */
typedef int pid_t;

/* Define to rpl_realloc if the replacement function should be used. */
/* #undef realloc */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to the type of an unsigned integer type of width exactly 16 bits if
 * such a type exists and the standard includes do not define it. */
/* #undef uint16_t */

/* Define to the type of an unsigned integer type of width exactly 32 bits if
 * such a type exists and the standard includes do not define it. */
/* #undef uint32_t */

/* Define to the type of an unsigned integer type of width exactly 64 bits if
 * such a type exists and the standard includes do not define it. */
/* #undef uint64_t */

/* Define to the type of an unsigned integer type of width exactly 8 bits if
 * such a type exists and the standard includes do not define it. */
/* #undef uint8_t */

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */

/* Define to empty if the keyword `volatile' does not work. Warning: valid
 * code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */
