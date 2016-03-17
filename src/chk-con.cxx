/*\
 * chk-con.cxx
 *
 * Copyright (c) 2015 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/

#define _WIN32_WINNT 0x0500

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <wchar.h>
#include "langdict.h"
#ifndef SPRTF
#define SPRTF printf
#endif

/* =======================================
from : https://msdn.microsoft.com/en-us/library/windows/desktop/dd319072(v=vs.85).aspx
int MultiByteToWideChar(
  _In_      UINT   CodePage,
  _In_      DWORD  dwFlags,
  _In_      LPCSTR lpMultiByteStr,
  _In_      int    cbMultiByte,
  _Out_opt_ LPWSTR lpWideCharStr,
  _In_      int    cchWideChar
);

from : https://msdn.microsoft.com/en-us/library/windows/desktop/dd374130(v=vs.85).aspx
int WideCharToMultiByte(
  _In_      UINT    CodePage,
  _In_      DWORD   dwFlags,
  _In_      LPCWSTR lpWideCharStr,
  _In_      int     cchWideChar,
  _Out_opt_ LPSTR   lpMultiByteStr,
  _In_      int     cbMultiByte,
  _In_opt_  LPCSTR  lpDefaultChar,
  _Out_opt_ LPBOOL  lpUsedDefaultChar
);

from : https://msdn.microsoft.com/en-us/library/ms683175(v=vs.85).aspx
HWND WINAPI GetConsoleWindow(void);


from : https://msdn.microsoft.com/query/dev10.query?appId=Dev10IDEF1&l=EN-US&k=k(%22WINCON%2fWRITECONSOLE%22);k(WRITECONSOLE);k(DevLang-%22C%2B%2B%22);k(TargetOS-WINDOWS)&rd=true
BOOL WINAPI WriteConsole(
  _In_             HANDLE  hConsoleOutput,
  _In_       const VOID    *lpBuffer,
  _In_             DWORD   nNumberOfCharsToWrite,
  _Out_            LPDWORD lpNumberOfCharsWritten,
  _Reserved_       LPVOID  lpReserved
);

from : https://msdn.microsoft.com/en-us/library/ms683167(v=vs.85).aspx
BOOL WINAPI GetConsoleMode(
  _In_  HANDLE  hConsoleHandle,
  _Out_ LPDWORD lpMode
);

  ======================================== */

#ifndef EndBuf
#define EndBuf(a) ( a + strlen(a) )
#endif

void show_hexified(char *from, int hlen, int iflag)
{
    static char _s_hex_pad[16 * 5];
    static char _s_asc_pad[32];
    int i, c, off;
    char *ph = _s_hex_pad;
    char *pa = _s_asc_pad;
    *ph = 0;
    off = 0;
    for (i = 0; i < hlen; i++) {
        c = ( from[i] & 0xff );
        sprintf(EndBuf(ph),"%02x ", c);
        if ((c < ' ')||(c >= 0x7f))
            c = '.';
        pa[off++] = (char)c;
        if (off == 16) {
            pa[off] = 0;
            SPRTF("%s %s\n", ph, pa);
            off = 0;
            *ph = 0;
        }
    }
    if (off) {
        pa[off] = 0;
        if (iflag & 0x01) {
            while(off < 16) {
                strcat(ph,"   ");
                off++;
            }
        }
        SPRTF("%s %s\n", ph, pa);
    }
}

static char buf[256];
static int set_new_codepage = 0;
static HWND hCon = 0;
static char *chin = "simplified Chinese: ??; traditional Chinese: ??";

static int stg_keys[] = {
    TIDY_LANGUAGE,
    FILE_CANT_OPEN,
    LINE_COLUMN_STRING,
    STRING_CONTENT_LOOKS,
    TC_STRING_VERS_A,
    TIDY_MESSAGE_TYPE_LAST
};

#define MX_BUF_SIZE 1024
static char _s_buf[MX_BUF_SIZE];
static char _s_buf2[MX_BUF_SIZE];
static wchar_t _s_wbuf[MX_BUF_SIZE*2];
void show_keys()
{
    int i, key, len, wlen, ilen, j;
    char *pen, *pzh;
    char *cp = _s_buf;
    char *cp2 = _s_buf2;
    wchar_t *wp = _s_wbuf;
    //DWORD wtn;
    for (i = 0; ; i++) {
        key = stg_keys[i];
        if (key == TIDY_MESSAGE_TYPE_LAST)
            break;
        pen = get_en_stg(key);
        pzh = get_zh_stg(key);
        if (!pen)
            pen = "<null>";
        if (!pzh)
            pzh = "<null>";
        len = sprintf(cp,"Key %d: '%s' | '%s'", key, pen, pzh);
        show_hexified(cp,len,1);
        printf("%s\n",cp);
        //if (hCon) {
        //    WriteConsole(hCon,cp,len,&wtn,0);
        //    printf("\n");
        //}
        wlen = MultiByteToWideChar(CP_UTF8,0,cp,len,wp,512);
        if (wlen > 0) {
            show_hexified((char *)wp,wlen*2,1);
            wprintf(L"%s\n",wp);
            ilen = WideCharToMultiByte(CP_ACP,0,wp,wlen,cp2,512,NULL,NULL);
            //if (len == ilen) {
                for(j = 0; j < len; j++) {
                    if (cp[j] != cp2[j])
                        break;
                }
            //}
        }
    }
}

void  get_console_info()
{
    HWND h = GetConsoleWindow();
    hCon = h;
    if (h) {
        DWORD mode;
        if (GetConsoleMode(h,&mode)) {


        }
    }

}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    UINT ccp, ncp;
    AllocConsole(); // add a console

    HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    int hCrt = _open_osfhandle((long) handle_out, _O_TEXT);
    FILE* hf_out = _fdopen(hCrt, "w");
    setvbuf(hf_out, NULL, _IONBF, 1);
    *stdout = *hf_out;

    HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
    hCrt = _open_osfhandle((long) handle_in, _O_TEXT);
    FILE* hf_in = _fdopen(hCrt, "r");
    setvbuf(hf_in, NULL, _IONBF, 128);
    *stdin = *hf_in;

    get_console_info();


    if (set_new_codepage) {
        ccp = GetConsoleOutputCP();
        SetConsoleOutputCP(65001);  // does NOT seem to do anything about UTF-8 correct display???
        ncp = GetConsoleOutputCP();
        printf("Changed console output from %u to %u\n", ccp, ncp);
    }

    // use the console just like a normal one - printf(), getchar(), ...
    show_keys();
    printf("printf output... <enter> key to continue : ");
    fread(buf,1,1,stdin);
    printf("bye...\n");
    return 0;
}

/* eof */
