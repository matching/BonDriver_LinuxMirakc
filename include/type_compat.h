// type_compat.h

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef bool BOOL;
typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef uint16_t WCHAR;
typedef const WCHAR * LPCWSTR;
typedef LPCWSTR LPCTSTR;

#define TRUE	1
#define FALSE	0

#define INFINITE	0xffffffff
#define WAIT_OBJECT_0	0x00000000
#define WAIT_ABANDONED	0x00000080
#define WAIT_TIMEOUT	0x00000102
