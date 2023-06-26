#ifndef _ERROR_H_
#define _ERROR_H_

enum ErrorCode
{
    E_NOT_ELF,
    INVALID_PROCESS_STATUS,
    INVALID_PERM,
    NO_FREE_PROCESS
};

// 有返回值的使用 try 返回值
// 没有返回值的使用 panic_on 获取是否成功

// OS error codes.

// Unspecified or unknown problem
#define E_UNSPECIFIED 1

// Environment doesn't exist or otherwise cannot be used in requested action
#define E_BAD_ENV 2

// Invalid parameter
#define E_INVAL 3

// Request failed due to memory shortage
#define E_NO_MEM 4

// Invalid syscall number
#define E_NO_SYS 5

// Attempt to create a new environment beyond the maximum allowed
#define E_NO_FREE_ENV 6

// Attempt to send to env that is not recving.
#define E_IPC_NOT_RECV 7

// File system error codes -- only seen in user-level

// No free space left on disk
#define E_NO_DISK 8

// Too many files are open
#define E_MAX_OPEN 9

// File or block not found
#define E_NOT_FOUND 10

// Bad path
#define E_BAD_PATH 11

// File already exists
#define E_FILE_EXISTS 12

// File not a valid executable
#define E_NOT_EXEC 13

#define EPERM 1    /* Operation not permitted */
#define ENOENT 2   /* No such file or directory */
#define ESRCH 3    /* No such process */
#define EINTR 4    /* Interrupted system call */
#define EIO 5      /* I/O error */
#define ENXIO 6    /* No such device or address */
#define E2BIG 7    /* Argument list too long */
#define ENOEXEC 8  /* Exec format error */
#define EBADF 9    /* Bad file number */
#define ECHILD 10  /* No child processes */
#define EAGAIN 11  /* Try again */
#define ENOMEM 12  /* Out of memory */
#define EACCES 13  /* Permission denied */
#define EFAULT 14  /* Bad address */
#define ENOTBLK 15 /* Block device required */
#define EBUSY 16   /* Device or resource busy */
#define EEXIST 17  /* File exists */
#define EXDEV 18   /* Cross-device link */
#define ENODEV 19  /* No such device */
#define ENOTDIR 20 /* Not a directory */
#define EISDIR 21  /* Is a directory */
#define EINVAL 22  /* Invalid argument */
#define ENFILE 23  /* File table overflow */
#define EMFILE 24  /* Too many open files */
#define ENOTTY 25  /* Not a typewriter */
#define ETXTBSY 26 /* Text file busy */
#define EFBIG 27   /* File too large */
#define ENOSPC 28  /* No space left on device */
#define ESPIPE 29  /* Illegal seek */
#define EROFS 30   /* Read-only file system */
#define EMLINK 31  /* Too many links */
#define EPIPE 32   /* Broken pipe */
#define EDOM 33    /* Math argument out of domain of func */
#define ERANGE 34  /* Math result not representable */

/*
 * A quick wrapper around function calls to propagate errors.
 * Use this with caution, as it leaks resources we've acquired so far.
 */
#define try(expr)       \
    do {                \
        int r = (expr); \
        if (r != 0)     \
            return r;   \
    } while (0)

#endif /* _ERROR_H_ */