/*
 * Modified for use with MPlayer, detailed CVS changelog at
 * http://www.mplayerhq.hu/cgi-bin/cvsweb.cgi/main/
 * $Id: win32.h 11593 2005-06-28 18:02:01Z courmisch $
 */

#ifndef loader_win32_h
#define loader_win32_h

#include <time.h>

#include "wine/windef.h"
#include "wine/winbase.h"
#include "com.h"

#ifdef AVIFILE
#ifdef __GNUC__
#include "avm_output.h"
#ifndef __cplusplus
#define printf(a, ...)  avm_printf("Win32 plugin", a, ## __VA_ARGS__)
#endif
#endif
#endif

extern void my_garbagecollection(void);

typedef struct {
    UINT             uDriverSignature;
    HINSTANCE        hDriverModule;
    DRIVERPROC       DriverProc;
    DWORD            dwDriverID;
} DRVR;

typedef DRVR  *PDRVR;
typedef DRVR  *NPDRVR;
typedef DRVR  *LPDRVR;

typedef struct tls_s tls_t;


extern void* LookupExternal(const char* library, int ordinal);
extern void* LookupExternalByName(const char* library, const char* name);

#endif
