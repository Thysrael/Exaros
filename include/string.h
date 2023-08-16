#ifndef __STRING_H
#define __STRING_H

#include "types.h"

int memcmp(const void *, const void *, u32);
void *memmove(void *, const void *, u32);
void *memset(void *, int, u32);
void *memcpy(void *dst, const void *src, u32 n);
char *safestrcpy(char *, const char *, int);
int strlen(const char *);
int strncmp(const char *, const char *, u32);
char *strncpy(char *, const char *, int);
void snstr(char *dst, wchar const *src, int len);
char *strchr(const char *s, char c);

#endif