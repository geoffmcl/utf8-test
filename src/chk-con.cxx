#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include "langdict.h"

static char buf[256];


static int stg_keys[] = {
    TIDY_LANGUAGE,
    FILE_CANT_OPEN,
    LINE_COLUMN_STRING,
    STRING_CONTENT_LOOKS,
    TC_STRING_VERS_A,
    TIDY_MESSAGE_TYPE_LAST
};

void show_keys()
{
    int i, key;
    char *pen, *pzh;
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
        printf("Key %d: '%s' | '%s'\n", key, pen, pzh);
    }
    //exit(0);
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    UINT ccp, ncp;
    AllocConsole();

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

    ccp = GetConsoleOutputCP();
    SetConsoleOutputCP(65001);  // does NOT seem to do anything about UTF-8 correct display???
    ncp = GetConsoleOutputCP();
    printf("Changed console output from %u to %u\n", ccp, ncp);

    // use the console just like a normal one - printf(), getchar(), ...
    show_keys();
    printf("printf output... <enter> key to continue : ");
    fread(buf,1,1,stdin);
    printf("bye...\n");
    return 0;
}

/* eof */
