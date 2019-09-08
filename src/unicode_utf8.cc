
/* unicode_utf8.cc

 * Copyright (c) 2015 - Geoff R. McLane
 * Licence: GNU GPL version 2
 
   from: https://gist.github.com/rechardchen/3321830
   
   2019-09-04 - Initial cut
*/
/*
    https://stijndewitt.com/2014/08/09/max-bytes-in-a-utf-8-char/
    There are a maximum of 4 bytes in a single UTF-8 encoded unicode character.
    And this is how the encoding scheme works in a nutshell.

    Bits Firstt	 Last     Bytes Byte 1	    Byte 2	    Byte 3	    Byte 4
    7	 U+0000	 U+007F	  1     0xxxxxxx
    11	 U+0080	 U+07FF	  2	    110xxxxx	10xxxxxx
    16	 U+0800	 U+FFFF	  3	    1110xxxx	10xxxxxx	10xxxxxx
    21	 U+10000 U+1FFFFF 4	    11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
    Source: Wikipedia (also confusingly showing 6 possible bytes when truly 4 is the maximum)
    They limited Unicode to a possible 1,112,064 valid code points.
    So they decided to increase the width of their fixed-width encoding to 32 bits,...
    
    So that is   0xFFFFFFFF, somewhat less that the MAX 
    codepoint of 0x001FFFFF, given above...

*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // for strdup(), ...
#ifdef WIN32
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#else
typedef unsigned int UINT;
typedef UINT DWORD;
#endif
#include <string>
#include <wchar.h>

using namespace std;

static const char *module = "unicode_utf8";

static const char *usr_input = 0;

void give_help(char *name)
{
    printf("%s: usage: [options] usr_input\n", module);
    printf("Options:\n");
    printf(" --help  (-h or -?) = This help and exit(0)\n");
    printf(" --show        (-s) = Show compiled widths, of char,int,etc - exit(1) for DEBUG only!\n");
    printf("\n");
    printf("  Give a codepoint, in hex, prefixed with 'U+' or '0x', else assumed\n");
    printf("  decimal value.\n");
    printf("  Try to convert the input to a code point, and convert\n");
    printf("  that to UTF-8, and show...");
}

//////////////////////////
void show_widths()
{
    printf("Size of 'char':     %u, bits %llu\n", (int)sizeof(char), sizeof(char) * 8);
    printf("Size of 'wchar_t':  %u, bits %llu\n", (int)sizeof(wchar_t), sizeof(wchar_t) * 8);
    printf("Size of 'uint16_t': %u, bits %llu\n", (int)sizeof(uint16_t), sizeof(uint16_t) * 8);
    printf("Size of 'int':      %u, bits %llu\n", (int)sizeof(int), sizeof(int) * 8);
    printf("Size of 'uint32_t': %u, bits %llu\n", (int)sizeof(uint32_t), sizeof(uint32_t) * 8);
    printf("Size of 'long':     %u, bits %llu\n", (int)sizeof(long), sizeof(long) * 8);
    printf("Size of 'uint64_t': %u, bits %llu\n", (int)sizeof(uint64_t), sizeof(uint64_t) * 8);
    exit(1);
}
///////////////////////////

int parse_args(int argc, char **argv)
{
    int i, i2, c;
    char *arg, *sarg;
    for (i = 1; i < argc; i++) {
        arg = argv[i];
        i2 = i + 1;
        if (*arg == '-') {
            sarg = &arg[1];
            while (*sarg == '-')
                sarg++;
            c = *sarg;
            switch (c) {
            case 'h':
            case '?':
                give_help(argv[0]);
                return 2;
                break;
            case 's':
                show_widths();
                // TODO: Other arguments
            default:
                printf("%s: Unknown argument '%s'. Try -? for help...\n", module, arg);
                return 1;
            }
        }
        else {
            // bear argument
            if (usr_input) {
                printf("%s: Already have input '%s'! What is this '%s'?\n", module, usr_input, arg);
                return 1;
            }
            usr_input = strdup(arg);
        }
    }
    if (!usr_input) {
        printf("%s: No user input found in command!\n", module);
        return 1;
    }
    return 0;
}

wstring UTF8toUnicode(const string& s)
{
    wstring ws;
    wchar_t wc;
    for( int i = 0;i < s.length(); )
    {
        char c = s[i];
        if ( (c & 0x80) == 0 )
        {
            wc = c;
            ++i;
        }
        else if ( (c & 0xE0) == 0xC0 )
        {
            wc = (s[i] & 0x1F) << 6;
            wc |= (s[i+1] & 0x3F);
            i += 2;
        }
        else if ( (c & 0xF0) == 0xE0 )
        {
            wc = (s[i] & 0xF) << 12;
            wc |= (s[i+1] & 0x3F) << 6;
            wc |= (s[i+2] & 0x3F);
            i += 3;
        }
        else if ( (c & 0xF8) == 0xF0 )
        {
            wc = (s[i] & 0x7) << 18;
            wc |= (s[i + 1] & 0x3F) << 12;
            wc |= (s[i + 2] & 0x3F) << 6;
            wc |= (s[i + 3] & 0x3F);
            i += 4;
        }
        else if ((c & 0xFC) == 0xF8)
        {
            wc = (s[i] & 0x3) << 24;
            wc |= (s[i + 1] & 0x3F) << 18;
            wc |= (s[i + 2] & 0x3F) << 12;
            wc |= (s[i + 3] & 0x3F) << 6;
            wc |= (s[i + 4] & 0x3F);
            i += 5;
        }
        else if ((c & 0xFE) == 0xFC)
        {
            wc = (s[i] & 0x1) << 30;
            wc |= (s[i + 1] & 0x3F) << 24;
            wc |= (s[i + 2] & 0x3F) << 18;
            wc |= (s[i + 3] & 0x3F) << 12;
            wc |= (s[i + 4] & 0x3F) << 6;
            wc |= (s[i + 5] & 0x3F);
            i += 6;
        }
        ws += wc;
    }
    return ws;
}

string UnicodeToUTF8(const wstring& ws)
{
    string s;
    for (int i = 0; i < ws.size(); ++i)
    {
        wchar_t wc = ws[i];
        if (0 <= wc && wc <= 0x7f)
        {
            s += (char)wc;
        }
        else if (0x80 <= wc && wc <= 0x7ff)
        {
            s += (0xc0 | (wc >> 6));
            s += (0x80 | (wc & 0x3f));
        }
        else if (0x800 <= wc && wc <= 0xffff)
        {
            s += (0xe0 | (wc >> 12));
            s += (0x80 | ((wc >> 6) & 0x3f));
            s += (0x80 | (wc & 0x3f));
        }
        else if (0x10000 <= wc && wc <= 0x1fffff)
        {
            s += (char)(0xf0); // | (wc >> 18));
            s += (0x80 | ((wc >> 12) & 0x3f));
            s += (0x80 | ((wc >> 6) & 0x3f));
            s += (0x80 | (wc & 0x3f));
        }
#if 0 /* DOES NOT EXIST */
        else if (0x200000 <= wc && wc <= 0x3ffffff)
        {
            s += (char)(0xf8); // | (wc >> 24));
            s += (char)(0x80); // | ((wc >> 18) & 0x3f));
            s += (0x80 | ((wc >> 12) & 0x3f));
            s += (0x80 | ((wc >> 6) & 0x3f));
            s += (0x80 | (wc & 0x3f));
        }
        else if (0x4000000 <= wc && wc <= 0x7fffffff)
        {
            s += (char)(0xfc); // | (wc >> 30));
            s += (char)(0x80); // | ((wc >> 24) & 0x3f));
            s += (char)(0x80); // | ((wc >> 18) & 0x3f));
            s += (0x80 | ((wc >> 12) & 0x3f));
            s += (0x80 | ((wc >> 6) & 0x3f));
            s += (0x80 | (wc & 0x3f));
        }
#endif /* DOES NOT EXIST */
        else {
            // too large - skip, or what...
        }
    }
    return s;
}

string CodepointToUTF8(unsigned int wc)
{
    string s;
    //for (int i = 0; i < ws.size(); ++i)
    {
        //wchar_t wc = ws[i];
        if (0 <= wc && wc <= 0x7f)
        {
            s += (char)wc;
        }
        else if (0x80 <= wc && wc <= 0x7ff)
        {
            s += (0xc0 | (wc >> 6));
            s += (0x80 | (wc & 0x3f));
        }
        else if (0x800 <= wc && wc <= 0xffff)
        {
            s += (0xe0 | (wc >> 12));
            s += (0x80 | ((wc >> 6) & 0x3f));
            s += (0x80 | (wc & 0x3f));
        }
        else if (0x10000 <= wc && wc <= 0x1fffff)
        {
            s += (0xf0 | (wc >> 18));
            s += (0x80 | ((wc >> 12) & 0x3f));
            s += (0x80 | ((wc >> 6) & 0x3f));
            s += (0x80 | (wc & 0x3f));
        }
#if 0 /* THESE DO NOT EXIST */
        else if (0x200000 <= wc && wc <= 0x3ffffff)
        {
            s += (0xf8 | (wc >> 24));
            s += (0x80 | ((wc >> 18) & 0x3f));
            s += (0x80 | ((wc >> 12) & 0x3f));
            s += (0x80 | ((wc >> 6) & 0x3f));
            s += (0x80 | (wc & 0x3f));
        }
        else if (0x4000000 <= wc && wc <= 0x7fffffff)
        {
            s += (0xfc | (wc >> 30));
            s += (0x80 | ((wc >> 24) & 0x3f));
            s += (0x80 | ((wc >> 18) & 0x3f));
            s += (0x80 | ((wc >> 12) & 0x3f));
            s += (0x80 | ((wc >> 6) & 0x3f));
            s += (0x80 | (wc & 0x3f));
        }
#endif /* DO NOT EXIST */
        else {
            // too large
            s = "";
        }
    }
    return s;
}

/////////////////////////////////////////////////////////////
/* hexadecimal string to decimal
 * from : https://stackoverflow.com/questions/10324/convert-a-hexadecimal-string-to-an-integer-efficiently-in-c/11068850
 */
static const long hextable[] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1, 0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,10,11,12,13,14,15,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

/**
* @brief convert a hexidecimal string to a signed long
* will not produce or process negative numbers except
* to signal error.
*
* @param hex without decoration, case insensitive.
*
* @return -1 on error, or result (max (sizeof(long)*8)-1 bits)
*/
long hexdec(/* unsigned */ const char *hex) {
    long ret = 0;
    while (*hex && ret >= 0) {
        ret = (ret << 4) | hextable[*hex++];
    }
    return ret;
}

// from: https://stackoverflow.com/questions/1394169/converting-hex-string-stored-as-chars-to-decimal-in-c

#define ISDIGIT(a) ((a >= '0') && (a <= '9'))
#define ISALPHAU(a) ((a >= 'A') && (a <= 'F'))
#define ISALPHAL(a) ((a >= 'a') && (a <= 'f'))
#define ISALPHA(a) (ISALPHAU(a) || ISALPHAL(a))
#define LOWERCASE(c) ((c) | ' ')

int collect_hex(const char * buf, size_t len, unsigned int *pui)
{
    unsigned int n = 0;
    if (!buf || len < 1 || !pui) {
        return 0;
    }
    long l = hexdec(buf);
    if (l >= 0) {
        n = (unsigned int)l;
        if (n > 0x1fffff) {
            printf("Value '%s' too large to be a codepoint! Max. 0x1fffff - %lu - got %lu\n", buf, 0x1fffff, n);
            return 0;
        }

        *pui = n;
        return 1;
    }

    const char *p = buf;
    int cnt = 0;
    while (*p) {
        if (ISDIGIT(*p)) {
            n = n * 16 + (*p++ - '0');
            cnt++;
        }
        else if (ISALPHAU(*p)) {
            n = n * 16 + (*p++ - 'A' + 10);
            cnt++;
        }
        else if (ISALPHAL(*p)) {
            n = n * 16 + (*p++ - 'a' + 10);
            cnt++;
        }
        else {
            printf("Buffer contains non-hexdecimal character!\n");
            return 0;
            break;
        }
        if (n > 0x1fffff) {
            printf("Value '%s' too large to be a codepoint! Max. 0x1fffff - %lu - got %lu\n", buf, 0x1fffff, n);
            return 0;
        }
    }
    if (cnt && (*p == 0)) {

        if (n > 0x1fffff) {
            printf("Value '%s' too large to be a codepoint! Max. 0x1fffff - %lu - got %lu\n", buf, 0x1fffff, n);
            return 0;
        }

        *pui = n;
        return cnt;
    }
    return 0;
}

void printf_hex(const char *buf, size_t len)
{
    size_t i;
    uint8_t c;
    printf("hex: ");
    for (i = 0; i < len; i++) {
        c = buf[i];
        printf("%02x ", (c & 0xff));
    }
}

int collect_dec(const char * buf, size_t len, unsigned int *pui)
{
    if (!buf || len < 1 || !pui) {
        return 0;
    }
    unsigned int n = 0;
    const char *p = buf;
    int cnt = 0;
    while (*p) {
        if (ISDIGIT(*p)) {
            n = n * 10 + (*p++ - '0');
            cnt++;
        }
        else {
            printf("Buffer contains non-decimal character!\n");
            return 0;
            break;
        }
    }
    if (cnt && (*p == 0)) {

        if (n > 0x1fffff) {
            printf("Value '%s' too large to be a codepoint! Max. 0x1fffff - %lu - got %lu\n", buf, 0x1fffff, n);
            return 0;
        }

        *pui = n;
        return cnt;
    }
    return 0;
}

int get_input()
{
    int iret = 1;
    unsigned int cp;
    if (!usr_input) {
        printf("%s: No user input found in command!\n", module);
        return 1;
    }
    size_t len = strlen(usr_input);
    char c, c1;
    if (!len) {
        printf("%s: No user input found in command!\n", module);
        return 1;
    }

    c = usr_input[0];
    c1 = (len > 1) ? usr_input[1] : 0;
    cp = 0;
    if (((c == 'U') && (c1 == '+')) ||
        ((c == '0') && ((c1 == 'x') || (c1 == 'X')) ))
    {
        // assume U+201C code point, or 0x201c
        if (len > 2) {
            if (collect_hex(&usr_input[2], (len - 2), &cp)) {
                string s = CodepointToUTF8(cp);
                wstring ws = UTF8toUnicode(s);
                if (ws.length() > 0) {
                    if (ws[0] == cp) {
                        iret = 0;
                    }
                    printf("Input: '%s', dec %u, Output: '%s', ", usr_input, cp, s.c_str());
                    printf_hex(s.c_str(), s.length());
                    printf("\n");
                }

            }
            else {
                printf("Failed to decode %s\n", usr_input);
                iret = 1;
            }
        }
        else {
            printf("Failed to decode %s\n", usr_input);
            iret = 1;
        }
    }
    else {
        // assume decimal only
        if (collect_dec(usr_input, len, &cp)) {
            string s = CodepointToUTF8(cp);
            wstring ws = UTF8toUnicode(s);
            if (ws.length() > 0) {
                if (ws[0] == cp) {
                    iret = 0;
                }
                printf("Input: '%s', hex 0x%x, Output: '%s', ", usr_input, cp, s.c_str());
                printf_hex(s.c_str(), s.length());
                printf("\n");
            }

        }
        else {
            printf("Failed to decode %s\n", usr_input);
            iret = 1;
        }
    }

    return iret;
}

// main() OS entry
int main( int argc, char **argv )
{
    int iret = 0;
    // show_widths();
    iret = parse_args(argc,argv);
    if (iret) {
        if (iret == 2)
            iret = 0;
        return iret;
    }

    iret = get_input(); // actions of app

    return iret;
}

/* eof */
