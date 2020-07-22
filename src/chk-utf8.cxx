/*\
 * chk-utf8.cxx
 *
 * Copyright (c) 2015 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/

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

// other includes


/*\
 **************************************************************************************************
 *                             Byte 1           2           3          4
 *                                  00..7F
 * 7	U+0000	    U+007F	    1	0xxxxxxx
 *                                  80..BF - non-initial bytes of multibyte code
 *                                  C2..FD - initial bytes of multibyte code (C0, C1 are not legal!)
 *                                  FE, FF - never used (so, free for byte-order marks).
 * 11	U+0080	    U+07FF	    2	110xxxxx	10xxxxxx
 * 16	U+0800	    U+FFFF	    3	1110xxxx	10xxxxxx	10xxxxxx
 * 21	U+10000	    U+1FFFFF	4	11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
 * 26	U+200000	U+3FFFFFF	5	111110xx	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx
 * 31	U+4000000	U+7FFFFFFF	6	1111110x	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx
 *
 **************************************************************************************************
 * from : html-tidy5/src/utf8.c
 *
 * <http://www.unicode.org/unicode/uni2errata/UTF-8_Corrigendum.html>
 *
 * Code point          1st byte    2nd byte    3rd byte    4th byte
 * ----------          --------    --------    --------    --------
 * U+0000..U+007F      00..7F
 * U+0080..U+07FF      C2..DF      80..BF
 * U+0800..U+0FFF      E0          A0..BF      80..BF
 * U+1000..U+FFFF      E1..EF      80..BF      80..BF
 * U+10000..U+3FFFF    F0          90..BF      80..BF      80..BF
 * U+40000..U+FFFFF    F1..F3      80..BF      80..BF      80..BF
 * U+100000..U+10FFFF  F4          80..8F      80..BF      80..BF
 * 
 **************************************************************************************************
 *
 * 1 byte  [00-7F]
 * 2 bytes [C2-DF][80-BF]
 * 3 bytes [E0   ][A0-BF][80-BF]
 * 3 bytes [E1-EC][80-BF][80-BF]
 * 3 bytes [ED   ][80-9F][80-BF]
 * 3 bytes [EE-EF][80-BF][80-BF]
 **************************************************************************************************
 * The character sequence U+D55C U+AD6D U+C5B4 (Korean "hangugeo",
 *  meaning "the Korean language") is encoded in UTF-8 as follows:
 *      ED 95 9C EA B5 AD EC 96 B4
 *
 *  The character sequence U+65E5 U+672C U+8A9E (Japanese "nihongo",
 *  meaning "the Japanese language") is encoded in UTF-8 as follows:
 *      E6 97 A5 E6 9C AC E8 AA 9E
 *
 ***************************************************************************************************
 * int _setmode(_fileno(stdout), _O_U16TEXT);
 * You can also pass _O_U16TEXT, _O_U8TEXT, or _O_WTEXT to enable Unicode mode
 * 
\*/
#ifdef WIN32
#ifndef DOSLIKE
#define DOSLIKE
#endif
#endif
#ifndef ISDIGIT
#define ISDIGIT(a) ( ( a >= '0' ) && ( a <= '9' ) )
#endif
#ifndef TEST_VERSION
#define TEST_VERSION "1.0.6"
#endif
#ifndef TEST_DATE
#define TEST_DATE "2020-07-22"
#endif

static const char *module = "chk-utf8";

static const char *usr_input = 0;
static bool use_printf = true; //false;
static bool use_wprintf = true;
static int utf_output = 0;
static int verbosity = 0;
static int err_max = 5;
#ifdef _WIN32
static int setconsole = 0;
#endif 

#define VERB1 ( verbosity >= 1 )
#define VERB2 ( verbosity >= 2 )
#define VERB5 ( verbosity >= 5 )
#define VERB9 ( verbosity >= 9 )

static int seq_errs = 0;
static int multi_count = 0;
static int char_count = 0;
static int line_count = 0;

void show_version()
{
    printf("%s: version: " TEST_VERSION ", date: " TEST_DATE "\n", module);
}

void give_help( char *name )
{
    show_version();
    printf("usage: %s [options] usr_input\n", module);
    printf("options:\n");
    printf(" --help  (-h or -?) = This help and exit(0)\n");
    printf(" --verb[n]     (-v) = Bump or set verbosity. n=0,1,2,5,9 (def=%d)\n", verbosity);
#ifdef _WIN32
    printf(" --console     (-c) = Add 'SetConsoleOutputCP(CP_UTF8)' before output, and reset after. (def=%d)\n", setconsole);
#endif
    printf(" Given an input file, scan it and report any invalid\n");
    printf(" UTF-8 byte sequences.\n");
    // TODO: More help
}

int parse_args( int argc, char **argv )
{
    int i,i2,c;
    char *arg, *sarg;
    for (i = 1; i < argc; i++) {
        arg = argv[i];
        i2 = i + 1;
        if (strcmp(arg, "--version") == 0) {
            show_version();
            return 2;
        }
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
            case 'v':
                sarg++;
                verbosity++;
                while (*sarg && !ISDIGIT(*sarg)) {
                    if (*sarg == 'v')
                        verbosity++;
                    sarg++;
                }
                if (ISDIGIT(*sarg))
                    verbosity = atoi(sarg);
                if (VERB1)
                    printf("Set verbosity to %d\n", verbosity);
                break;
#ifdef _WIN32
            case 'c':
                setconsole = 1;
                if (VERB1)
                    printf("Set console to CP_UTF8, %d, and reset.\n", CP_UTF8);
                break;
#endif
                // TODO: Other arguments
            default:
                printf("%s: Unknown argument '%s'. Try -? for help...\n", module, arg);
                return 1;
            }
        } else {
            // bear argument
            if (usr_input) {
                printf("%s: Already have input '%s'! What is this '%s'?\n", module, usr_input, arg );
                return 1;
            }
            usr_input = strdup(arg);
        }
    }
    if (!usr_input) {
        give_help(argv[0]);
        printf("%s: Error: No user input found in command!\n", module);
        return 1;
    }
    return 0;
}

void *allocMem(size_t n)
{
	void *s;
	if (!(s = malloc(n))) {
        fprintf(stderr,"%s: Memory FAILED on %d bytes!\n", module, (int)n);
        exit(1);
    }
	return s;
}				/* allocMem */

// SetConsoleOutputCP
void eb_puts(const char *s)
{
#ifdef DOSLIKE
    size_t len;
	wchar_t *chars = NULL;
	DWORD written, mode;
	HANDLE output_handle;
	int needed;
	output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (GetConsoleMode(output_handle, &mode) == 0) {
		puts(s);
		return;
	}
    // SetConsoleOutputCP(???);
	needed = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	if (needed == 0) {
		return;
	}
	//Add space for the newline
    len = sizeof(wchar_t) * (needed + 2);
	chars = (wchar_t *) allocMem(len);
    memset(chars,0,len);
	MultiByteToWideChar(CP_UTF8, 0, s, -1, chars, needed);
	//chars[needed] = 0;
	//chars[needed - 1] = L'\r';
	//chars[needed] = L'\n';
	//chars[needed + 1] = L'\0';
	//WriteConsoleW(output_handle, (void *)chars, needed + 1, &written, NULL);
    if (use_wprintf) {
        wprintf(L"%s",chars);
    } else {
        WriteConsoleW(output_handle, (void *)chars, needed, &written, NULL);
    }
	free(chars);
#else
	puts(s);
#endif
}				/* eb_puts */

#ifdef _WIN32
static UINT oldcp = 0;
static int prevmode = 0;
BOOL CALLBACK EnumCodePagesProc(LPTSTR lpCodePageString)
{
    if (lpCodePageString && *lpCodePageString)
        printf("%s ",lpCodePageString);
    return TRUE;
}

void enumCodePages() 
{
    DWORD dwFlags = CP_INSTALLED;
    BOOL res = EnumSystemCodePages(EnumCodePagesProc, // _In_ CODEPAGE_ENUMPROC lpCodePageEnumProc,
        dwFlags);
}

void set_console()
{
    if (setconsole) {
        // enumCodePages();
        oldcp = GetConsoleOutputCP();
        SetConsoleOutputCP(CP_UTF8);
        if (VERB5)
            fprintf(stderr, "%s: Set console to UTF-8 %d, from %d.\n", module, CP_UTF8, oldcp);
        //prevmode = _setmode(_fileno(stdout), _O_U16TEXT);
    }
}
void reset_console()
{
    if (setconsole && oldcp) {
        SetConsoleOutputCP(oldcp);
        if (VERB5)
            fprintf(stderr,"%s: Reset console to %d.\n", module, oldcp);
    }
    oldcp = 0;
}
#endif // #ifdef _WIN32

void printf_hex( const char *buf, size_t len )
{
    size_t i;
    uint8_t c;
    printf("hex: ");
    for (i = 0; i < len; i++) {
        c = buf[i];
        printf("%02x ", (c & 0xff));
    }
}

////////////////////////////////////////////////////////////////////
//// from : http://stackoverflow.com/questions/2948308/how-do-i-read-utf-8-characters-via-a-pointer

#define IS_IN_RANGE(c, f, l)    (((c) >= (f)) && ((c) <= (l)))

//u_long readNextChar(char *& p, int len) 
int getNextSeqLen(char *& p, int datalen, u_long *puc)
{  

    u_char c1, c2, *ptr = (u_char*)p;
    u_long uc = 0;
    int seqlen;
    // int datalen = ... available length of p ...;    
    if (datalen < 1)
    {
        // malformed data, do something !!!
        return (u_long)-1;
    }

    c1 = ptr[0];

    if ((c1 & 0x80) == 0)
    {
        uc = (u_long)(c1 & 0x7F);
        seqlen = 1;
    }
    else if ((c1 & 0xE0) == 0xC0)
    {
        uc = (u_long)(c1 & 0x1F);
        seqlen = 2;
    }
    else if ((c1 & 0xF0) == 0xE0)
    {
        uc = (u_long)(c1 & 0x0F);
        seqlen = 3;
    }
    else if ((c1 & 0xF8) == 0xF0)
    {
        uc = (u_long)(c1 & 0x07);
        seqlen = 4;
    }
    else
    {
        // malformed data, do something !!!
        return (u_long)-1;
    }

    if (seqlen > datalen)
    {
        // malformed data, do something !!!
        return (u_long)-1;
    }

    for (int i = 1; i < seqlen; ++i)
    {
        c1 = ptr[i];

        if ((c1 & 0xC0) != 0x80)
        {
            // malformed data, do something !!!
            return (u_long)-1;
        }
    }

    switch (seqlen)
    {
    case 2:
    {
        c1 = ptr[0];

        if (!IS_IN_RANGE(c1, 0xC2, 0xDF))
        {
            // malformed data, do something !!!
            return (u_long)-1;
        }

        break;
    }

    case 3:
    {
        c1 = ptr[0];
        c2 = ptr[1];

        switch (c1)
        {
        case 0xE0:
            if (!IS_IN_RANGE(c2, 0xA0, 0xBF))
            {
                // malformed data, do something !!!
                return (u_long)-1;
            }
            break;

        case 0xED:
            if (!IS_IN_RANGE(c2, 0x80, 0x9F))
            {
                // malformed data, do something !!!
                return (u_long)-1;
            }
            break;

        default:
            if (!IS_IN_RANGE(c1, 0xE1, 0xEC) && !IS_IN_RANGE(c1, 0xEE, 0xEF))
            {
                // malformed data, do something !!!
                return (u_long)-1;
            }
            break;
        }

        break;
    }

    case 4:
    {
        c1 = ptr[0];
        c2 = ptr[1];

        switch (c1)
        {
        case 0xF0:
            if (!IS_IN_RANGE(c2, 0x90, 0xBF))
            {
                // malformed data, do something !!!
                return (u_long)-1;
            }
            break;

        case 0xF4:
            if (!IS_IN_RANGE(c2, 0x80, 0x8F))
            {
                // malformed data, do something !!!
                return (u_long)-1;
            }
            break;

        default:
            if (!IS_IN_RANGE(c1, 0xF1, 0xF3))
            {
                // malformed data, do something !!!
                return (u_long)-1;
            }
            break;
        }

        break;
    }
    }

    for (int i = 1; i < seqlen; ++i)
    {
        uc = ((uc << 6) | (u_long)(ptr[i] & 0x3F));
    }

    if (puc)
        *puc = uc;      // unicodeChar; 
    p += seqlen;
    // return uc;
    return seqlen;
}

int chk_buffer_sequences( uint8_t *buf, long ilen )
{
    char *p = (char *)buf;
    int c, i, res, len = ilen;
    char *nxt;
    bool in_err = false;
    for (i = 0; i < len; i++) {
        u_long uc = 0;
        c = buf[i];
        if (c == '\n')
            line_count++;
        nxt = p;
        if ( (res = getNextSeqLen( p, len - i, &uc )) > 0 ) {
            res--;
            if (res) {
                i += res;
                multi_count++;
            }
            in_err = false;
        } else {
            if (!in_err)
                seq_errs++;
            p++;    // bump just one char
            in_err = true;
        }
        char_count++;
    }
    printf("%s: Processed %d lines, found %d multi-bytes, total %d chars... errs=%d\n", module,
        line_count, multi_count, char_count, seq_errs );
    return seq_errs;
}


////////////////////////////////////////////////////////////////////
#ifdef _WIN32

int chk_utf8_buffer(uint8_t *buf, long len)
{
    int iret = 0;
    uint8_t utf8[8];
    long i, off, ulen, cnt;
    uint8_t c, d;
    long line, col;
    line = 1;
    col = 1;
    set_console();
    if (VERB2) {
        if (use_printf) {
            printf("%s: Using simple printf to output the utf-8\n", module);
        }
        else {
            printf("%s: Using eb_puts output the UNICODE %s\n", module,
                (use_wprintf ? "using wprintf" : "using WriteConsoleW"));
        }
    }
    for (i = 0; i < len; i++) {
        c = buf[i];
        if (( c == '\n' ) || ( c == '\r' )) {
            if (( c == '\n' ) && ((i + 1) < len) && (buf[i+1] == '\r'))
                i++;
            line++;
            col = 1;
            continue;
        }
        if (c & 0x80) {
            off = 0;
            utf8[off++] = c;
            if (( c >= 0xc2 ) && ( c <= 0xfd )) {
                // C2..FD - initial bytes of multibyte code (C0, C1 are not legal!)
                // get lenght
                d = 0x80;
                ulen = 0;
                while (c & d) {
                    ulen++;
                    d = d >> 1;
                }
                if (ulen >= 2) {
                    ulen--;
                    cnt = 1;
                    while (ulen) {
                        i++;
                        c = buf[i];
                        utf8[off++] = c;
                        cnt++;
                        if (( c & 0x80 ) && !( c & 0x40 )) {
                            // valid utf-8 byte
                        } else {
                            fprintf(stderr,"%s: Invalid %lu utf-8 byte 0x%2x!\n", module, cnt, c & 0xff);
                            iret = 1;
                            break;
                        }
                        ulen--;
                    }
                    if (iret)
                        break;
                    utf8[off] = 0;
                    if (VERB2) {
                        printf("line:%d col:%d len:%d '", line, col, off);
                        if (use_printf) {
                            printf("%s", utf8);
                        }
                        else {
                            eb_puts((const char *)utf8); // display the UTF-8 sequence
                        }
                        printf("' ");
                        printf_hex((const char *)utf8, off);
                        printf("\n");
                    }
                    utf_output++;
                } else {
                    fprintf(stderr,"%s:%d:%d: Invalid first utf-8 byte 0x%2x!\n", module, line, col, c & 0xff);
                    iret = 1;
                    break;
                }
            } else {
                //fprintf(stderr,"%s: Invalid first utf-8 byte 0x%2x!\n", module, c & 0xff);
                fprintf(stderr, "%s:%d:%d: Invalid first utf-8 byte 0x%2x!\n", module, line, col, c & 0xff);
                iret = 1;
                break;
            }
            col += off; // bytes decoded
        } else {
            col++;
        }
    }
    if (!VERB2) {
        if (utf_output) {
            printf("%s: Would have output %d utf-8 sequences if '-v2' is set...\n", module, utf_output);
        }
    }
    reset_console();
    return iret;
}

#endif // #ifdef _WIN32


int chk_utf8()
{
    int iret = 0;
    const char *file = usr_input;
    uint8_t *buf;
    long len, res;
    FILE *fp = fopen(file,"rb");
    if (!fp) {
        fprintf(stderr,"%s: Unable to open file '%s'!\n", module, file);
        return 1;
    }
    if (fseek(fp,0,SEEK_END)) {
        fclose(fp);
        fprintf(stderr,"%s: Unable to seek to end of file '%s'!\n", module, file);
        return 1;
    }
    len = ftell(fp);
    if (fseek(fp,0,SEEK_SET)) {
        fclose(fp);
        fprintf(stderr,"%s: Unable to seek to end of file '%s'!\n", module, file);
        return 1;
    }
    if (len == 0) {
        fclose(fp);
        fprintf(stderr,"%s: File '%s' has zero length!\n", module, file);
        return 1;
    }
    if (len == -1) {
        fclose(fp);
        fprintf(stderr,"%s: File '%s' is too large!\n", module, file);
        return 1;
    }
    buf = (uint8_t *)malloc(len + 8);
    if (!buf) {
        fprintf(stderr,"%s: Loading file '%s', but memory failed on %lu bytes!\n", module, file, len+8);
        fclose(fp);
        return 1;
    }
    fprintf(stderr,"%s: Loading file '%s', %lu bytes!\n", module, file, len);
    res = (long)fread(buf,1,len,fp);
    fclose(fp);
    if (res != len) {
        fprintf(stderr,"%s: Loading FAILED! got %lu bytes!\n", module, res);
        free(buf);
        return 1;
    }
    iret |= chk_buffer_sequences(buf,len);
#ifdef _WIN32
    iret |= chk_utf8_buffer(buf,len);
#endif // #ifdef _WIN32
    
    free(buf);

#ifdef _WIN32
    if (utf_output)
        fprintf(stderr,"%s: Found %d multibyte UTF-8 characters. ", module, utf_output);
    else
        fprintf(stderr, "%s: No multibyte UTF-8 characters found in %d chars. ", module, (int)len);
    fprintf(stderr, "exit(%d)\n", iret);
#else // !_WIN32
    if (multi_count)
        fprintf(stderr, "%s: Found %d multibyte UTF-8 characters. ", module, multi_count);
    else
        fprintf(stderr, "%s: No multibyte UTF-8 characters found in %d chars. ", module, (int)len);
    fprintf(stderr, "exit(%d)\n", iret);
#endif // _WIN32 y/n

    return iret;
}

// Sample files
// F:\Projects\utf8\data\Chinese2.html
// main() OS entry
int main( int argc, char **argv )
{
    int iret = 0;
    iret = parse_args(argc,argv);
    if (iret) {
        if (iret == 2)
            iret = 0;
        return iret;
    }

    iret = chk_utf8(); // action of app

    return iret;
}

#ifdef INC_TIDY_UTF8_C
///////////////////////////////////////////////////////////////////////////

/* utf8.c -- convert characters to/from UTF-8

(c) 1998-2007 (W3C) MIT, ERCIM, Keio University
See tidy.h for the copyright notice.

Uses public interfaces to abstract input source and output
sink, which may be user supplied or either FILE* or memory
based Tidy implementations.  Encoding support is uniform
regardless of I/O mechanism.

Note, UTF-8 encoding, by itself, does not affect the actual
"codepoints" of the underlying character encoding.  In the
cases of ASCII, Latin1, Unicode (16-bit, BMP), these all
refer to ISO-10646 "codepoints".  For anything else, they
refer to some other "codepoint" set.

Put another way, UTF-8 is a variable length method to
represent any non-negative integer value.  The glyph
that a integer value represents is unchanged and defined
externally (e.g. by ISO-10646, Big5, Win1252, MacRoman,
Latin2-9, and so on).

Put still another way, UTF-8 is more of a _transfer_ encoding
than a _character_ encoding, per se.
*/

//#include "tidy.h"
//#include "forward.h"
//#include "utf8.h"

/* hack for gnu sys/types.h file which defines uint and ulong */

//#if defined(BE_OS) || defined(SOLARIS_OS) || defined(BSD_BASED_OS) || defined(OSF_OS) || defined(IRIX_OS) || defined(AIX_OS)
#include <sys/types.h>
//#endif
#include <assert.h>
#if !defined(HPUX_OS) && !defined(CYGWIN_OS) && !defined(MAC_OS_X) && !defined(BE_OS) && !defined(SOLARIS_OS) && !defined(BSD_BASED_OS) && !defined(OSF_OS) && !defined(IRIX_OS) && !defined(AIX_OS) && !defined(LINUX_OS)
# undef uint
typedef unsigned int uint;
#endif
#if defined(HPUX_OS) || defined(CYGWIN_OS) || defined(MAC_OS) || defined(BSD_BASED_OS) || defined(_WIN32)
# undef ulong
typedef unsigned long ulong;
#endif

typedef enum
{
    no,
    yes
} Bool;

typedef unsigned char byte;

typedef uint tchar;         /* single, full character */
typedef char tmbchar;       /* single, possibly partial character */
#ifndef TMBSTR_DEFINED
typedef tmbchar* tmbstr;    /* pointer to buffer of possibly partial chars */
typedef const tmbchar* ctmbstr; /* Ditto, but const */
#define NULLSTR (tmbstr)""
#define TMBSTR_DEFINED
#endif

                                /** End of input "character" */
#define EndOfStream (~0u)

/* Internal symbols are prefixed to avoid clashes with other libraries */
#define TYDYAPPEND(str1,str2) str1##str2
#define TY_(str) TYDYAPPEND(prvTidy,str)

/*
UTF-8 encoding/decoding functions
Return # of bytes in UTF-8 sequence; result < 0 if illegal sequence

Also see below for UTF-16 encoding/decoding functions

References :

1) UCS Transformation Format 8 (UTF-8):
ISO/IEC 10646-1:1996 Amendment 2 or ISO/IEC 10646-1:2000 Annex D
<http://anubis.dkuug.dk/JTC1/SC2/WG2/docs/n1335>
<http://www.cl.cam.ac.uk/~mgk25/ucs/ISO-10646-UTF-8.html>

Table 4 - Mapping from UCS-4 to UTF-8

2) Unicode standards:
<http://www.unicode.org/unicode/standard/standard.html>

3) Legal UTF-8 byte sequences:
<http://www.unicode.org/unicode/uni2errata/UTF-8_Corrigendum.html>

Code point          1st byte    2nd byte    3rd byte    4th byte
----------          --------    --------    --------    --------
U+0000..U+007F      00..7F
U+0080..U+07FF      C2..DF      80..BF
U+0800..U+0FFF      E0          A0..BF      80..BF
U+1000..U+FFFF      E1..EF      80..BF      80..BF
U+10000..U+3FFFF    F0          90..BF      80..BF      80..BF
U+40000..U+FFFFF    F1..F3      80..BF      80..BF      80..BF
U+100000..U+10FFFF  F4          80..8F      80..BF      80..BF

The definition of UTF-8 in Annex D of ISO/IEC 10646-1:2000 also
allows for the use of five- and six-byte sequences to encode
characters that are outside the range of the Unicode character
set; those five- and six-byte sequences are illegal for the use
of UTF-8 as a transformation of Unicode characters. ISO/IEC 10646
does not allow mapping of unpaired surrogates, nor U+FFFE and U+FFFF
(but it does allow other noncharacters).

4) RFC 2279: UTF-8, a transformation format of ISO 10646:
<http://www.ietf.org/rfc/rfc2279.txt>

5) UTF-8 and Unicode FAQ:
<http://www.cl.cam.ac.uk/~mgk25/unicode.html>

6) Markus Kuhn's UTF-8 decoder stress test file:
<http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt>

7) UTF-8 Demo:
<http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-demo.txt>

8) UTF-8 Sampler:
<http://www.columbia.edu/kermit/utf8.html>

9) Transformation Format for 16 Planes of Group 00 (UTF-16):
ISO/IEC 10646-1:1996 Amendment 1 or ISO/IEC 10646-1:2000 Annex C
<http://anubis.dkuug.dk/JTC1/SC2/WG2/docs/n2005/n2005.pdf>
<http://www.cl.cam.ac.uk/~mgk25/ucs/ISO-10646-UTF-16.html>

10) RFC 2781: UTF-16, an encoding of ISO 10646:
<http://www.ietf.org/rfc/rfc2781.txt>

11) UTF-16 invalid surrogate pairs:
<http://www.unicode.org/unicode/faq/utf_bom.html#16>

UTF-16       UTF-8          UCS-4
D83F DFF*    F0 9F BF B*    0001FFF*
D87F DFF*    F0 AF BF B*    0002FFF*
D8BF DFF*    F0 BF BF B*    0003FFF*
D8FF DFF*    F1 8F BF B*    0004FFF*
D93F DFF*    F1 9F BF B*    0005FFF*
D97F DFF*    F1 AF BF B*    0006FFF*
...
DBBF DFF*    F3 BF BF B*    000FFFF*
DBFF DFF*    F4 8F BF B*    0010FFF*

* = E or F

1010  A
1011  B
1100  C
1101  D
1110  E
1111  F

*/

#define kNumUTF8Sequences        7
#define kMaxUTF8Bytes            4

#define kUTF8ByteSwapNotAChar    0xFFFE
#define kUTF8NotAChar            0xFFFF

#define kMaxUTF8FromUCS4         0x10FFFF

#define kUTF16SurrogatesBegin    0x10000
#define kMaxUTF16FromUCS4        0x10FFFF

/* UTF-16 surrogate pair areas */
#define kUTF16LowSurrogateBegin  0xD800
#define kUTF16LowSurrogateEnd    0xDBFF
#define kUTF16HighSurrogateBegin 0xDC00
#define kUTF16HighSurrogateEnd   0xDFFF


/* offsets into validUTF8 table below */
static const int offsetUTF8Sequences[kMaxUTF8Bytes + 1] =
{
    0, /* 1 byte */
    1, /* 2 bytes */
    2, /* 3 bytes */
    4, /* 4 bytes */
    kNumUTF8Sequences /* must be last */
};

static const struct validUTF8Sequence
{
    uint lowChar;
    uint highChar;
    int  numBytes;
    byte validBytes[8];
} validUTF8[kNumUTF8Sequences] =
{
    /*   low       high   #bytes  byte 1      byte 2      byte 3      byte 4 */
    { 0x0000,   0x007F,   1,{ 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x0080,   0x07FF,   2,{ 0xC2, 0xDF, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00 } },
    { 0x0800,   0x0FFF,   3,{ 0xE0, 0xE0, 0xA0, 0xBF, 0x80, 0xBF, 0x00, 0x00 } },
    { 0x1000,   0xFFFF,   3,{ 0xE1, 0xEF, 0x80, 0xBF, 0x80, 0xBF, 0x00, 0x00 } },
    { 0x10000,  0x3FFFF,  4,{ 0xF0, 0xF0, 0x90, 0xBF, 0x80, 0xBF, 0x80, 0xBF } },
    { 0x40000,  0xFFFFF,  4,{ 0xF1, 0xF3, 0x80, 0xBF, 0x80, 0xBF, 0x80, 0xBF } },
    { 0x100000, 0x10FFFF, 4,{ 0xF4, 0xF4, 0x80, 0x8F, 0x80, 0xBF, 0x80, 0xBF } }
};

//int TY_DecodeUTF8BytesToChar(uint* c, uint firstByte, ctmbstr successorBytes,
//    TidyInputSource* inp, int* count)
int TY_DecodeUTF8BytesToChar(uint* c, uint firstByte, ctmbstr successorBytes,
    void* inp, int* count)
{
    byte tempbuf[10];
    byte *buf = &tempbuf[0];
    uint ch = 0, n = 0;
    int i, bytes = 0;
    Bool hasError = no;

    if (successorBytes)
        buf = (byte*)successorBytes;

    /* special check if we have been passed an EOF char */
    if (firstByte == EndOfStream)
    {
        /* at present */
        *c = firstByte;
        *count = 1;
        return 0;
    }

    ch = firstByte; /* first byte is passed in separately */

    if (ch <= 0x7F) /* 0XXX XXXX one byte */
    {
        n = ch;
        bytes = 1;
    }
    else if ((ch & 0xE0) == 0xC0)  /* 110X XXXX  two bytes */
    {
        n = ch & 31;
        bytes = 2;
    }
    else if ((ch & 0xF0) == 0xE0)  /* 1110 XXXX  three bytes */
    {
        n = ch & 15;
        bytes = 3;
    }
    else if ((ch & 0xF8) == 0xF0)  /* 1111 0XXX  four bytes */
    {
        n = ch & 7;
        bytes = 4;
    }
    else if ((ch & 0xFC) == 0xF8)  /* 1111 10XX  five bytes */
    {
        n = ch & 3;
        bytes = 5;
        hasError = yes;
    }
    else if ((ch & 0xFE) == 0xFC)  /* 1111 110X  six bytes */
    {
        n = ch & 1;
        bytes = 6;
        hasError = yes;
    }
    else
    {
        /* not a valid first byte of a UTF-8 sequence */
        n = ch;
        bytes = 1;
        hasError = yes;
    }

    /* successor bytes should have the form 10XX XXXX */

    /* If caller supplied buffer, use it.  Else see if caller
    ** supplied an input source, use that.
    */
    if (successorBytes)
    {
        for (i = 0; i < bytes - 1; ++i)
        {
            if (!buf[i] || (buf[i] & 0xC0) != 0x80)
            {
                hasError = yes;
                bytes = i + 1;
                break;
            }
            n = (n << 6) | (buf[i] & 0x3F);
        }
    }
#if 0 // 000000000000000000000000000000000000000
    else if (inp)
    {
        for (i = 0; i < bytes - 1 && !inp->eof(inp->sourceData); ++i)
        {
            int b = inp->getByte(inp->sourceData);
            buf[i] = (tmbchar)b;

            /* End of data or illegal successor byte value */
            if (b == EOF || (buf[i] & 0xC0) != 0x80)
            {
                hasError = yes;
                bytes = i + 1;
                if (b != EOF)
                    inp->ungetByte(inp->sourceData, buf[i]);
                break;
            }
            n = (n << 6) | (buf[i] & 0x3F);
        }
    }
#endif
    else if (bytes > 1)
    {
        hasError = yes;
        bytes = 1;
    }

    if (!hasError && ((n == kUTF8ByteSwapNotAChar) || (n == kUTF8NotAChar)))
        hasError = yes;

    if (!hasError && (n > kMaxUTF8FromUCS4))
        hasError = yes;

#if 0 /* Breaks Big5 D8 - DF */
    if (!hasError && (n >= kUTF16LowSurrogateBegin) && (n <= kUTF16HighSurrogateEnd))
        /* unpaired surrogates not allowed */
        hasError = yes;
#endif

    if (!hasError)
    {
        int lo, hi;

        lo = offsetUTF8Sequences[bytes - 1];
        hi = offsetUTF8Sequences[bytes] - 1;

        /* check for overlong sequences */
        if ((n < validUTF8[lo].lowChar) || (n > validUTF8[hi].highChar))
            hasError = yes;
        else
        {
            hasError = yes; /* assume error until proven otherwise */

            for (i = lo; i <= hi; i++)
            {
                int tempCount;
                byte theByte;

                for (tempCount = 0; tempCount < bytes; tempCount++)
                {
                    if (!tempCount)
                        theByte = (tmbchar)firstByte;
                    else
                        theByte = buf[tempCount - 1];

                    if (theByte >= validUTF8[i].validBytes[(tempCount * 2)] &&
                        theByte <= validUTF8[i].validBytes[(tempCount * 2) + 1])
                        hasError = no;
                    if (hasError)
                        break;
                }
            }
        }
    }

#if 1 && defined(_DEBUG)
    if (hasError)
    {
        /* debug */
        fprintf(stderr, "UTF-8 decoding error of %d bytes : ", bytes);
        fprintf(stderr, "0x%02x ", firstByte);
        for (i = 1; i < bytes; i++)
            fprintf(stderr, "0x%02x ", buf[i - 1]);
        fprintf(stderr, " = U+%04ulx\n", n);
    }
#endif

    *count = bytes;
    *c = n;
    if (hasError)
        return -1;
    return 0;
}

//int TY_EncodeCharToUTF8Bytes(uint c, tmbstr encodebuf,
//    TidyOutputSink* outp, int* count)
int TY_EncodeCharToUTF8Bytes(uint c, tmbstr encodebuf,
    void* outp, int* count)
{
    byte tempbuf[10] = { 0 };
    byte* buf = &tempbuf[0];
    int bytes = 0;
    Bool hasError = no;

    if (encodebuf)
        buf = (byte*)encodebuf;

    if (c <= 0x7F)  /* 0XXX XXXX one byte */
    {
        buf[0] = (tmbchar)c;
        bytes = 1;
    }
    else if (c <= 0x7FF)  /* 110X XXXX  two bytes */
    {
        buf[0] = (tmbchar)(0xC0 | (c >> 6));
        buf[1] = (tmbchar)(0x80 | (c & 0x3F));
        bytes = 2;
    }
    else if (c <= 0xFFFF)  /* 1110 XXXX  three bytes */
    {
        buf[0] = (tmbchar)(0xE0 | (c >> 12));
        buf[1] = (tmbchar)(0x80 | ((c >> 6) & 0x3F));
        buf[2] = (tmbchar)(0x80 | (c & 0x3F));
        bytes = 3;
        if (c == kUTF8ByteSwapNotAChar || c == kUTF8NotAChar)
            hasError = yes;
#if 0 /* Breaks Big5 D8 - DF */
        else if (c >= kUTF16LowSurrogateBegin && c <= kUTF16HighSurrogateEnd)
            /* unpaired surrogates not allowed */
            hasError = yes;
#endif
    }
    else if (c <= 0x1FFFFF)  /* 1111 0XXX  four bytes */
    {
        buf[0] = (tmbchar)(0xF0 | (c >> 18));
        buf[1] = (tmbchar)(0x80 | ((c >> 12) & 0x3F));
        buf[2] = (tmbchar)(0x80 | ((c >> 6) & 0x3F));
        buf[3] = (tmbchar)(0x80 | (c & 0x3F));
        bytes = 4;
        if (c > kMaxUTF8FromUCS4)
            hasError = yes;
    }
    else if (c <= 0x3FFFFFF)  /* 1111 10XX  five bytes */
    {
        buf[0] = (tmbchar)(0xF8 | (c >> 24));
        buf[1] = (tmbchar)(0x80 | (c >> 18));
        buf[2] = (tmbchar)(0x80 | ((c >> 12) & 0x3F));
        buf[3] = (tmbchar)(0x80 | ((c >> 6) & 0x3F));
        buf[4] = (tmbchar)(0x80 | (c & 0x3F));
        bytes = 5;
        hasError = yes;
    }
    else if (c <= 0x7FFFFFFF)  /* 1111 110X  six bytes */
    {
        buf[0] = (tmbchar)(0xFC | (c >> 30));
        buf[1] = (tmbchar)(0x80 | ((c >> 24) & 0x3F));
        buf[2] = (tmbchar)(0x80 | ((c >> 18) & 0x3F));
        buf[3] = (tmbchar)(0x80 | ((c >> 12) & 0x3F));
        buf[4] = (tmbchar)(0x80 | ((c >> 6) & 0x3F));
        buf[5] = (tmbchar)(0x80 | (c & 0x3F));
        bytes = 6;
        hasError = yes;
    }
    else
        hasError = yes;

#if 0 // 000000000000000000000000000
    /* don't output invalid UTF-8 byte sequence to a stream */
    if (!hasError && outp != NULL)
    {
        int ix;
        for (ix = 0; ix < bytes; ++ix)
            outp->putByte(outp->sinkData, buf[ix]);
    }
#endif

#if 1 && defined(_DEBUG)
    if (hasError)
    {
        int i;
        fprintf(stderr, "UTF-8 encoding error for U+%x : ", c);
        for (i = 0; i < bytes; i++)
            fprintf(stderr, "0x%02x ", buf[i]);
        fprintf(stderr, "\n");
    }
#endif

    *count = bytes;
    if (hasError)
        return -1;
    return 0;
}


/* return one less than the number of bytes used by the UTF-8 byte sequence */
/* str points to the UTF-8 byte sequence */
/* the Unicode char is returned in *ch */
uint TY_GetUTF8(ctmbstr str, uint *ch)
{
    uint n;
    int bytes;

    int err;

    bytes = 0;

    /* first byte "str[0]" is passed in separately from the */
    /* rest of the UTF-8 byte sequence starting at "str[1]" */
    err = TY_DecodeUTF8BytesToChar(&n, str[0], str + 1, NULL, &bytes);
    //err = TY_DecodeUTF8BytesToChar(&n, str[0], str + 1, &bytes);
    if (err)
    {
#if 1 && defined(_DEBUG)
        fprintf(stderr, "pprint UTF-8 decoding error for U+%x : ", n);
#endif
        n = 0xFFFD; /* replacement char */
    }

    *ch = n;
    return bytes - 1;
}

/* store char c as UTF-8 encoded byte stream */
tmbstr TY_PutUTF8(tmbstr buf, uint c)
{
    int err, count = 0;

    err = TY_EncodeCharToUTF8Bytes(c, buf, NULL, &count);
    if (err)
    {
#if 1 && defined(_DEBUG)
        fprintf(stderr, "pprint UTF-8 encoding error for U+%x : ", c);
#endif
        /* replacement char 0xFFFD encoded as UTF-8 */
        buf[0] = (byte)0xEF;
        buf[1] = (byte)0xBF;
        buf[2] = (byte)0xBD;
        count = 3;
    }

    buf += count;
    return buf;
}

Bool    TY_IsValidUTF16FromUCS4(tchar ucs4)
{
    return (ucs4 <= kMaxUTF16FromUCS4) ? yes : no;
}

Bool    TY_IsHighSurrogate(tchar ch)
{
    return (ch >= kUTF16HighSurrogateBegin && ch <= kUTF16HighSurrogateEnd) ? yes : no;
}
Bool    TY_IsLowSurrogate(tchar ch)
{
    return (ch >= kUTF16LowSurrogateBegin && ch <= kUTF16LowSurrogateEnd) ? yes : no;
}

tchar   TY_CombineSurrogatePair(tchar high, tchar low)
{
    assert(TY_IsHighSurrogate(high) && TY_IsLowSurrogate(low));
    return (((low - kUTF16LowSurrogateBegin) * 0x400) +
        high - kUTF16HighSurrogateBegin + 0x10000);
}

Bool    TY_IsValidCombinedChar(tchar ch);

Bool   TY_SplitSurrogatePair(tchar utf16, tchar* low, tchar* high)
{
    Bool status = (TY_IsValidCombinedChar(utf16) && high && low) ? yes : no;
    if (status)
    {
        *low = (utf16 - kUTF16SurrogatesBegin) / 0x400 + kUTF16LowSurrogateBegin;
        *high = (utf16 - kUTF16SurrogatesBegin) % 0x400 + kUTF16HighSurrogateBegin;
    }
    return status;
}

Bool    TY_IsValidCombinedChar(tchar ch)
{
    return (ch >= kUTF16SurrogatesBegin &&
        (ch & 0x0000FFFE) != 0x0000FFFE &&
        (ch & 0x0000FFFF) != 0x0000FFFF) ? yes : no;
}

Bool    TY_IsCombinedChar(tchar ch)
{
    return (ch >= kUTF16SurrogatesBegin) ? yes : no;
}

///////////////////////////////////////////////////////////////////////////
#endif // #ifdef INC_TIDY_UTF8_C

// eof = chk-utf8.cxx
