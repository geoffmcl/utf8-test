/*\
 * utf8-test.cxx
 *
 * Copyright (c) 2014-2015 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/
/*\
 * from : http://utfcpp.sourceforge.net/ with some modifications
 *
 * Essentially read an input file, line by line
 * Search for an invalid utf-8 sequences, give length (up to first invlid)
 *
\*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h> // for strdup(), ...
#include <stdlib.h> // for atoi, ... in linux
#include <stdint.h>
#include "utf8.h"

using namespace std;

#ifndef SPRTF
#define SPRTF printf
#endif
#ifndef EndBuf
#define EndBuf(a) ( a + strlen(a) )
#endif
#ifndef ISDIGIT
#define ISDIGIT(a) ( ( a >= '0' ) && ( a <= '9' ) )
#endif

static const char *module = "utf8-test";

static const char *usr_input = 0;
static bool do_full_test = false;
static int verbosity = 1;

#define VERB1 ( verbosity >= 1 )
#define VERB2 ( verbosity >= 2 )
#define VERB5 ( verbosity >= 5 )
#define VERB9 ( verbosity >= 9 )

#ifdef _MSC_VER
#define M_IS_DIR _S_IFDIR
#else // !_MSC_VER
#define M_IS_DIR S_IFDIR
#endif

enum DiskType {
    DT_NONE,
    DT_FILE,
    DT_DIR
};


static struct stat buf;
DiskType is_file_or_directory32 ( const char * path )
{
    if (!path)
        return DT_NONE;
	if (stat(path,&buf) == 0)
	{
		if (buf.st_mode & M_IS_DIR)
			return DT_DIR;
		else
			return DT_FILE;
	}
	return DT_NONE;
}


void give_help( char *name )
{
    printf("%s: usage: [options] usr_input\n", module);
    printf("Options:\n");
    printf(" --help  (-h or -?) = This help and exit(2)\n");
    printf(" --verb[n]     (-v) = Bump or set verbosity. n=0,1,2,5,9 (def=%d)\n", verbosity );
    printf(" --full        (-f) = Do full test even if sample passes simple verification. (def=%s)\n",
        (do_full_test ? "On" : "Off"));
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
            case 'f':
                do_full_test = true;
                break;
            default:
                printf("%s: Unknown argument '%s'. Tyr -? for help...\n", module, arg);
                return 1;
            }
        } else {
            // bear argument
            if (usr_input) {
                printf("%s: Already have input '%s'! What is this '%s'?\n", module, usr_input, arg );
                return 1;
            }
            if (is_file_or_directory32(arg) != DT_FILE) {
                printf("%s: Unable to 'stat' file '%s'!\n", module, arg );
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

// forward references
std::string get_hex_stg(const char *from, int hlen, int flag = 0);

bool IsLittleEndian()
{
    static const int EndianTest = 1;
    return (*((char *) &EndianTest ) != 0);
}

inline uint32_t bswap_32(uint32_t x) {
    x = ((x >>  8) & 0x00FF00FFL) | ((x <<  8) & 0xFF00FF00L);
    x = (x >> 16) | (x << 16);
    return x;
}

#define MSWAP32(a) IsLittleEndian() ? bswap_32(a) : a

std::string get_hex_stg(const char *from, int hlen, int flag)
{
    static char _s_hex_pad[16 * 5];
    static char _s_asc_pad[32];
    ostringstream hex;
    int i, c, off;
    char *ph = _s_hex_pad;
    char *pa = _s_asc_pad;
    *ph = 0;
    off = 0;
    if ((flag & 0x01)&&(hlen > 4)) {
        uint32_t *pui = (uint32_t *)from;
        i = MSWAP32(*pui);
        SPRTF("First uint32 = %u\n", i);
    }
    for (i = 0; i < hlen; i++) {
        c = ( from[i] & 0xff );
        if (flag & 0x04) {
            if (i & 1) {
                sprintf(EndBuf(ph),"%02X ", c);
            } else {
                sprintf(EndBuf(ph),"%02X", c);
            }
        } else {
            sprintf(EndBuf(ph),"%02x ", c);
        }
        if ((c < ' ')||(c >= 0x7f))
            c = '.';
        pa[off++] = (char)c;
        if (off == 16) {
            pa[off] = 0;
            hex << ph << " " << pa << endl;
            //SPRTF("%s %s\n", ph, pa);
            off = 0;
            *ph = 0;
        }
    }
    if (off) {
        pa[off] = 0;
        if (flag & 0x02) {
            while(off < 16) {
                strcat(ph,"   ");
                off++;
            }
        }
        hex << ph << " " << pa;
        //SPRTF("%s %s\n", ph, pa);
    }
    return hex.str();
}


int utf8_test(const char * file)
{

    const char* test_file_path = file;
    // Open the test file (contains UTF-8 encoded text)
    ifstream fs8(test_file_path);
    if (!fs8.is_open()) {
        cerr << "Could not open " << test_file_path << endl;
        return 1;
    }

    unsigned line_count = 1;
    int error_count = 0;
    size_t len, rem, dist;
    string line, ok, end;
    // Play with all the lines in the file
    while (getline(fs8, line)) {
        len = line.size();  // NOTE: This is BYTE count
        string::iterator line_bgn = line.begin();
        string::iterator line_end = line.end();
        dist = utf8::distance(line_bgn, line_end);  // NOTE: This a CHARACTER length
        // check for invalid utf-8 (for a simple yes/no check, there is also utf8::is_valid function)
        // string::iterator end_it = utf8::find_invalid(line.begin(), line.end());
        string::iterator end_it = utf8::find_invalid(line_bgn, line_end);
        //if (end_it != line.end()) {
        if (end_it != line_end) {
            if (VERB1) {
                // ok = string(line.begin(), end_it);
                ok = string(line_bgn, end_it);
                cout << "Invalid UTF-8 encoding detected at line " << line_count << ", offet " << ok.size() << endl;
                if (VERB2) {
                    cout << "This part is fine: " << ok << "\n";
                    end = line.substr(ok.size());
                    rem = end.size();
                    if (VERB5) {
                        if (rem >= 4) {
                            ok = get_hex_stg(end.c_str(),4,4);
                            cout << "Followed by hex: " << ok << endl;
                        }
                    }
                }
            }
            error_count++;
        }

        // Get the line length (at least for the valid part)
        // int length = utf8::distance(line.begin(), end_it);
        size_t length = utf8::distance(line_bgn, end_it);   // NOTE: This is CHARACTER count, NOT byte count
        if (VERB1) {
            cout << "Line " << line_count << " is " << len << " bytes, " << dist << " chars";
            if (dist == length) {
                if (length) {
                    cout << ", all valid";
                    if (length == len)
                        cout << ", but no multibyte chars";
                }
                else {
                    cout << ", zero length";
                }
            } else {
                cout << ", but only " << length << " valid";
            }
            cout << "." << endl;
        }

        if (length) {
            // Convert it to utf-16, just for FUN
            vector<unsigned short> utf16line;
            utf8::utf8to16(line_bgn, end_it, back_inserter(utf16line));
            // utf8::utf8to16(line.begin(), end_it, back_inserter(utf16line));

            // And back to utf-8, just for FUN
            string utf8line;
            utf8::utf16to8(utf16line.begin(), utf16line.end(), back_inserter(utf8line));

            // Confirm that the conversion went OK:
            if (utf8line != string(line_bgn, end_it)) {
                if (VERB1) {
                    cerr << "Error in UTF-16 conversion at line: " << line_count << endl;
                }
            }
        }

        line_count++;
    }
    if (VERB5 && error_count) {
        cout << "Character codes for UTF-8 must be in the range: U+0000 to U+10FFFF." << endl;
    }
    return error_count;
}

bool valid_utf8_file(const char* file_name)
{
    ifstream ifs(file_name);
    if (!ifs)
        return false; // even better, throw here

    istreambuf_iterator<char> it(ifs.rdbuf());
    istreambuf_iterator<char> eos;

    return utf8::is_valid(it, eos);
}

// main() OS entry
// tests:
// F:\Projects\utf8\data\UTF-8-test.txt
// F:\Projects\tidy-html5\test\input5\in_88-1.html
// F:\Projects\utf8\data\Tidy-004-chineseU.txt
int main( int argc, char **argv )
{
    int iret = 0;
    iret = parse_args(argc,argv);
    if (iret) {
        if (iret == 2)
            iret = 0;
        return iret;
    }

    if (valid_utf8_file(usr_input)) {
        if (VERB1) {
            printf("%s: File '%s' appears to contain valid UTF-8 text\n", module, usr_input);
        }
    } else {
        if (VERB1) {
            printf("%s: File '%s' appears to contain INVALID UTF-8 text\n", module, usr_input);
        }
        iret = 1;
        do_full_test = true;
    }

    if (do_full_test) {
        iret |= utf8_test(usr_input);
    }

    return iret;
}


// eof = utf8-test.cxx
