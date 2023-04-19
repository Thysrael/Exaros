#ifndef _ERROR_H_
#define _ERROR_H_

enum ErrorCode
{
    E_NOT_ELF,
    INVALID_PROCESS_STATUS,
    INVALID_PERM,
    NO_FREE_PROCESS
};

#define E_NO_MEM 4

#endif /* _ERROR_H_ */