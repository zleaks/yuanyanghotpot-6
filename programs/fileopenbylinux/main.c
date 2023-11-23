#include "ws2tcpip.h"
#include "iphlpapi.h"
#include "winternl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <icmpapi.h>
#include <limits.h>

#include <windows.h>

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(linuxopen);

BOOL FileExists(LPCWSTR szFilename)
{
   WIN32_FIND_DATAW entry;
   HANDLE hFile;

   hFile = FindFirstFileW(szFilename, &entry);
   FindClose(hFile);

   return (hFile != INVALID_HANDLE_VALUE);
}

static BOOL openPathByNemo(LPCSTR unix_path){
    char *unix_cmd;
    char nemo[] = "nemo";
    int cmdlen,syscmdlen,ret;
    if(unix_path != NULL)
    {
        cmdlen = lstrlenA(nemo);
        syscmdlen = cmdlen+1+1+lstrlenA(unix_path)+1+1;//cmd "unix_path"\0
        unix_cmd = HeapAlloc(GetProcessHeap(),0,syscmdlen);
        lstrcpyA(unix_cmd,nemo);
        lstrcpyA(unix_cmd+cmdlen," \"");
        lstrcpyA(unix_cmd+cmdlen+2,unix_path);
        lstrcpyA(unix_cmd+syscmdlen-2,"\"");
        *(unix_cmd+syscmdlen-1)=0;
        TRACE("call nemo(%s)\n",unix_cmd);
        ret = system(unix_cmd);
        HeapFree(GetProcessHeap(),0,unix_cmd);
        if(ret != -1 && ret != 127)
        {
            TRACE("nemo exec OK\n");
            return TRUE;
        }else
        {
            TRACE("nemo exec failed\n");
            return FALSE;
        }
    }
    return FALSE;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR cmd, int show)
{
    LPCWSTR fileName = NULL;
    BOOL isFileExist = FALSE;
    int retval=0;
    char *unix_path,*unix_cmd;
    char xdg[] = "xdg-open";
    int cmdlen,syscmdlen,ret;

    LPCWSTR cmdline=GetCommandLineW();
    TRACE("check cmd=%s\n",debugstr_w(cmdline));
    WCHAR delimiter, *ptr;

    /* skip white space */
    while (*cmdline == ' ') cmdline++;

    /* skip executable name */
    delimiter = (*cmdline == '"' ? '"' : ' ');

    if (*cmdline == delimiter) cmdline++;

    while (*cmdline && *cmdline != delimiter) cmdline++;

    if (*cmdline == delimiter) cmdline++;

    while (*cmdline == ' ') cmdline++;

    ptr = cmdline;
    while (*ptr == ' ' || *ptr == '-' || *ptr == '/')
    {
        WCHAR option;

        if (*ptr++ == ' ') continue;

        option = *ptr;
        if (option) ptr++;
        while (*ptr == ' ') ptr++;
    }

    if (*cmdline)
    {
        /* file name is passed in the command line */
        if (cmdline[0] == '"')
        {
            WCHAR* wc;
            cmdline++;
            wc=cmdline;
            /* Note: Double-quotes are not allowed in Windows filenames */
            while (*wc && *wc != '"') wc++;
            /* On Windows notepad ignores further arguments too */
            *wc = 0;
        }
        if (FileExists(cmdline))
        {
            isFileExist = TRUE;
            fileName = cmdline;
        }
        TRACE("filename=%s isExist=%d\n",debugstr_w(fileName),isFileExist);
    }


    if(!isFileExist){
        TRACE("file not exist!! file=%s\n",debugstr_w(fileName));
        exit(1);
    }

    unix_path = wine_get_unix_file_name(fileName);
    if(!unix_path){
        exit(1);
    }
    TRACE("open unix path=%s\n",unix_path);
    cmdlen = lstrlenA(xdg);
    syscmdlen = cmdlen+1+1+lstrlenA(unix_path)+1+1;//cmd "unix_path"\0
    unix_cmd = HeapAlloc(GetProcessHeap(),0,syscmdlen);
    lstrcpyA(unix_cmd,xdg);
    lstrcpyA(unix_cmd+cmdlen," \"");
    lstrcpyA(unix_cmd+cmdlen+2,unix_path);
    lstrcpyA(unix_cmd+syscmdlen-2,"\"");
    *(unix_cmd+syscmdlen-1)=0;
    TRACE("call system(%s)\n",unix_cmd);
    ret = system(unix_cmd);
    if(ret != -1 && ret != 127)
    {
        TRACE("system exec OK\n");
        retval = 0;
    }else
    {
        TRACE("system exec failed, try to show in nemo\n");
        if(openPathByNemo(unix_path)){
            retval = 0;
        }else{
            retval = 2;
        }
    }
    HeapFree(GetProcessHeap(),0,unix_path);
    HeapFree(GetProcessHeap(),0,unix_cmd);
    exit(retval);
}
