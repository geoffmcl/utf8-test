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
#define TEST_VERSION "1.0.1"
#endif
#ifndef TEST_DATE
#define TEST_DATE "2016-11-26"
#endif

static const char *module = "chk-utf8";

static const char *usr_input = 0;
static bool use_printf = true; //false;
static bool use_wprintf = true;
static int utf_output = 0;
static int verbosity = 0;
static int err_max = 5;

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
        fprintf(stderr,"%s: Memory FAILED on %lu bytes!\n", module, n);
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
    // enumCodePages();
    oldcp = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
    fprintf(stderr,"%s: Set console to UTF-8 %d, from %d.\n", module, CP_UTF8, oldcp);
    //prevmode = _setmode(_fileno(stdout), _O_U16TEXT);
}
void reset_console()
{
    if (oldcp) {
        SetConsoleOutputCP(oldcp);
        fprintf(stderr,"%s: Reset console to %d.\n", module, oldcp);
    }
    oldcp = 0;
}

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
int getNextSeqLen(char *& p, int len, u_long *puc) 
{  
    u_char c1, c2, first, *ptr = (u_char *) p;
    u_long uc = 0;
    int seqlen = 0;
    int datalen = len; // available length of p ...;    
    int i, j;

    if( datalen < 1 )
    {
        // malformed data, do something !!!
        if (VERB1) {
            printf("Err: No data length!\n");
        }
        return -1;
    }

    c1 = ptr[0];
    first = c1;
    if( (c1 & 0x80) == 0 )
    {
        uc = (u_long) (c1 & 0x7F);
        seqlen = 1;
    }
    else if( (c1 & 0xE0) == 0xC0 )
    {
        uc = (u_long) (c1 & 0x1F);
        seqlen = 2;
    }
    else if( (c1 & 0xF0) == 0xE0 )
    {
        uc = (u_long) (c1 & 0x0F);
        seqlen = 3;
    }
    else if( (c1 & 0xF8) == 0xF0 )
    {
        uc = (u_long) (c1 & 0x07);
        seqlen = 4;
    }
    else
    {
        // malformed data, do something !!!
        if (VERB1) {
            printf("Err: First char %02x NOT a valid lead byte!\n", (c1 & 0xff));
        }
        return -1;
    }

    if( seqlen > datalen )
    {
        // malformed data, do something !!!
        if (VERB1) {
            printf("Err: Sequence len %d GT data len %d\n", seqlen, datalen);
        }
        return -1;
    }

    for(i = 1; i < seqlen; ++i)
    {
        c1 = ptr[i];

        if( (c1 & 0xC0) != 0x80 )
        {
            // malformed data, do something !!!
            if ((VERB9) || ((seq_errs < err_max) && VERB1)) 
            {
                switch (seqlen)
                {
                case 2:
                    printf("Err: First uchar %02x, len %d, followed by %02x is invalid!\n", (first & 0xff),
                        seqlen, (c1 & 0xff));
                    break;
                case 3:
                case 4:
                    if (i > 1) {
                        printf("Err: First uchars %02x, ", (first & 0xff));
                        for (j = 1; j < i; j++) {
                            printf("%02x, ", (ptr[j] & 0xff));
                        }
                        printf("len %d, followed by %02x is invalid!\n", seqlen, (c1 & 0xff));
                    }
                    else {
                        printf("Err: First uchar %02x, len %d, followed by %02x is invalid!\n", (first & 0xff),
                            seqlen, (c1 & 0xff));
                    }
                    break;
                }
            }
            return -1;
        }
    }

    switch( seqlen )
    {
        case 2:
        {
            c1 = ptr[0];

            if( !IS_IN_RANGE(c1, 0xC2, 0xDF) )
            {
                // malformed data, do something !!!
                return -1;
            }

            break;
        }

        case 3:
        {
            c1 = ptr[0];
            c2 = ptr[1];

            if( ((c1 == 0xE0) && !IS_IN_RANGE(c2, 0xA0, 0xBF)) ||
                ((c1 == 0xED) && !IS_IN_RANGE(c2, 0x80, 0x9F)) ||
                (!IS_IN_RANGE(c1, 0xE1, 0xEC) && !IS_IN_RANGE(c1, 0xEE, 0xEF)) )
            {
                // malformed data, do something !!!
                return -1;
            }

            break;
        }

        case 4:
        {
            c1 = ptr[0];
            c2 = ptr[1];

            if( ((c1 == 0xF0) && !IS_IN_RANGE(c2, 0x90, 0xBF)) ||
                ((c1 == 0xF4) && !IS_IN_RANGE(c2, 0x80, 0x8F)) ||
                !IS_IN_RANGE(c1, 0xF1, 0xF3) )
            {
                // malformed data, do something !!!
                return -1;
            }

            break;
        }
    }

    for(int i = 1; i < seqlen; ++i)
    {
        uc = ((uc << 6) | (u_long)(ptr[i] & 0x3F));
    }
    *puc = uc;      // unicodeChar; 
    p += seqlen;
    return seqlen;  
}

int chk_buffer_sequences( uint8_t *buf, long ilen )
{
    char *p = (char *)buf;
    int c, i, res, len = ilen;
    char *nxt;
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
        } else {
            seq_errs++;
            p++;    // bump just one char
        }
        char_count++;
    }
    printf("%s: Processed %d lines, found %d multi-bytes, total %d chars... errs=%d\n", module,
        line_count, multi_count, char_count, seq_errs );
    return seq_errs;
}


////////////////////////////////////////////////////////////////////

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
    if (use_printf) {
        printf("%s: Using simple printf to output the utf-8\n", module );
    } else {
        printf("%s: Using eb_puts output the UNICODE %s\n", module,
           (use_wprintf ? "using wprintf" : "using WriteConsoleW") );
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
                    printf("line:%d col:%d len:%d '", line, col, off);
                    if (use_printf) {
                        printf("%s", utf8);
                    } else {
                        eb_puts((const char *)utf8); // display the UTF-8 sequence
                    }
                    printf("' ");
                    printf_hex( (const char *)utf8, off );
                    printf("\n");
                    utf_output++;
                } else {
                    fprintf(stderr,"%s: Invalid first utf-8 byte 0x%2x!\n", module, c & 0xff);
                    iret = 1;
                    break;
                }
            } else {
                fprintf(stderr,"%s: Invalid first utf-8 byte 0x%2x!\n", module, c & 0xff);
                iret = 1;
                break;
            }
            col += off; // bytes decoded
        } else {
            col++;
        }
    }
    reset_console();
    return iret;
}


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
    res = fread(buf,1,len,fp);
    fclose(fp);
    if (res != len) {
        fprintf(stderr,"%s: Loading FAILED! got %lu bytes!\n", module, res);
        free(buf);
        return 1;
    }
    iret |= chk_buffer_sequences(buf,len);
    iret |= chk_utf8_buffer(buf,len);
    free(buf);
    fprintf(stderr,"%s: Output %d UTF-8 characters found.\n", module, utf_output);
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


// eof = chk-utf8.cxx
