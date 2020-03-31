#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <sysexits.h>
#include <stdlib.h>

#ifdef NDEBUG
#define ENABLE_DEBUG false
#else
#define ENABLE_DEBUG true
#endif

/*
 *  EX_USAGE -- The command was used incorrectly, e.g., with
 *      the wrong number of arguments, a bad flag, a bad
 *      syntax in a parameter, or whatever.
 *  EX_DATAERR -- The input data was incorrect in some way.
 *      This should only be used for user's data & not
 *      system files.
 *  EX_NOINPUT -- An input file (not a system file) did not
 *      exist or was not readable.  This could also include
 *      errors like "No message" to a mailer (if it cared
 *      to catch it).
 *  EX_NOUSER -- The user specified did not exist.  This might
 *      be used for mail addresses or remote logins.
 *  EX_NOHOST -- The host specified did not exist.  This is used
 *      in mail addresses or network requests.
 *  EX_UNAVAILABLE -- A service is unavailable.  This can occur
 *      if a support program or file does not exist.  This
 *      can also be used as a catchall message when something
 *      you wanted to do doesn't work, but you don't know
 *      why.
 *  EX_SOFTWARE -- An internal software error has been detected.
 *      This should be limited to non-operating system related
 *      errors as possible.
 *  EX_OSERR -- An operating system error has been detected.
 *      This is intended to be used for such things as "cannot
 *      fork", "cannot create pipe", or the like.  It includes
 *      things like getuid returning a user that does not
 *      exist in the passwd file.
 *  EX_OSFILE -- Some system file (e.g., /etc/passwd, /etc/utmp,
 *      etc.) does not exist, cannot be opened, or has some
 *      sort of error (e.g., syntax error).
 *  EX_CANTCREAT -- A (user specified) output file cannot be
 *      created.
 *  EX_IOERR -- An error occurred while doing I/O on some file.
 *  EX_TEMPFAIL -- temporary failure, indicating something that
 *      is not really an error.  In sendmail, this means
 *      that a mailer (e.g.) could not create a connection,
 *      and the request should be reattempted later.
 *  EX_PROTOCOL -- the remote system returned something that
 *      was "not possible" during a protocol exchange.
 *  EX_NOPERM -- You did not have sufficient permission to
 *      perform the operation.  This is not intended for
 *      file system problems, which should use NOINPUT or
 *      CANTCREAT, but rather for higher level permissions.
 */

// Use if statement instead of pre-processor directives because this way
// the compiler will still parse it. Since ENABLE_DEBUG is a compile-time
// constant, the compiler should remove it entirely from no debug builds
#define debug(MSG, ...) \
    do {                            \
        if(ENABLE_DEBUG)            \
            fprintf(stderr,         \
                "[DBG (%s:%d)] " MSG "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define log(MSG, ...) \
    do {                            \
        fprintf(stdout,             \
                "[LOG (%s:%d)] " MSG "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define log_err(MSG, ...) \
    do {                            \
        fprintf(stderr,             \
                "\033[1;91m[ERR (%s:%d)]\033[0;91m " MSG "\033[0m\n", \
                __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define ensure(COND, MSG, ...)                          \
    do {                                                \
        if(!(COND)) {                                   \
            log_err(MSG, ##__VA_ARGS__);                \
            goto error;                                 \
        }                                               \
    } while(0)

#define ensure_exit(COND, STATUS, MSG, ...)                         \
    do {                                                \
        if(!(COND)) {                                   \
            log_err(MSG, ##__VA_ARGS__);                \
            perror(""); \
            exit(STATUS);                                   \
        }                                               \
    } while(0)

#define ensure_mem(PTR) \
    do { ensure_exit((PTR) != NULL, EX_OSERR, "Out of memory"); } while(0)

#define message(MSG, ...) \
    do {                                    \
        printf(MSG "\n", ##__VA_ARGS__);    \
    } while(0)

#endif
