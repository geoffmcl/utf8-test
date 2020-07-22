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
#ifdef HAVE_DIRENT_H
#include <string>
#include <vector>
#include <dirent.h>
#endif // #ifdef HAVE_DIRENT_H
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

#ifdef _MSC_VER
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
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
    MDT_NONE = 0,
    MDT_FILE = 1,
    MDT_DIR = 2
};


static struct stat buf;
DiskType is_file_or_directory32 ( const char * path )
{
    if (!path)
    {
        return MDT_NONE;
    }

	if (stat(path,&buf) == 0)
	{
		if (buf.st_mode & M_IS_DIR)
			return MDT_DIR;
		else
			return MDT_FILE;
	}
	return MDT_NONE;
}


void give_help( char *name )
{
    printf("%s: usage: [options] usr_input\n", module);
    printf("Options:\n");
    printf(" --help  (-h or -?) = This help and exit(0)\n");
    printf(" --verb[n]     (-v) = Bump or set verbosity. n=0,1,2,5,9 (def=%d)\n", verbosity );
    printf(" --full        (-f) = Do full test even if sample passes simple verification. (def=%s)\n",
        (do_full_test ? "On" : "Off"));
    printf("\n");
#ifdef HAVE_DIRENT_H
    printf(" If given a directory, or wildcard, search and process all files matching.\n");
#endif // 
    // TODO: More help
}

#ifdef HAVE_DIRENT_H
typedef std::vector<std::string> vSTG;

vSTG vFiles;

int has_wild(char* arg)
{
    size_t i, len = strlen(arg);
    char ch;
    for (i = 0; i < len; i++) {
        ch = arg[i];
        if (ch == '*')
            return 1;
        if (ch == '?')
            return 1;
    }
    return 0;
}

void fix_directory(std::string& dir)
{
    char ps = '\\';
    size_t pos = dir.rfind(ps);
    if (pos == std::string::npos) {
        ps = '/';
        pos = dir.rfind(ps);
    }
    if (pos != dir.size())
        dir += ps;
}

int expand_dir(char* dir)
{
    int count = 0;
    DIR* dp = opendir(dir);
    if (!dp) {
        printf("%s: unable to open directory '%s'\n", module, dir);
        return 0;
    }
    std::string s(dir);
    std::string ff;
    struct dirent* pe;
    DiskType typ;
    vSTG vDirs;
    fix_directory(s);
    uint64_t dirs = 0;
    while ((pe = readdir(dp)) != 0) {
        if (strcmp(pe->d_name, ".") == 0)
            continue;
        if (strcmp(pe->d_name, "..") == 0)
            continue;
        ff = s;
        ff += pe->d_name;
        //typ = dep;
        //while (typ--) {
            // printf(" ");
        //}

        typ = is_file_or_directory32(ff.c_str());
        if (typ == MDT_DIR) {
            vDirs.push_back(ff); // if/when recursive desired...
            dirs++;
        }
        else if (typ == MDT_FILE) {
            vFiles.push_back(ff);
            count++;
        }
    }
    //if (rcursive) {

    //}


    return count;
}

int is_match(const char* name, const char* wild)
{
    size_t len1 = strlen(wild);
    size_t len2 = strlen(name);
    size_t i, j = 0;
    size_t k;
    char c1, c2;
    // for each character in the wild
    // match to approp char, or chars, in name
    for (i = 0; i < len1; i++) {
        c1 = wild[i];
        c2 = name[j];
        k = i + 1;
        if (c1 == c2) {
            j++;    // advance name
            if (j < len2)
                continue;
            // name has expired
            k = i + 1;
            if (k < len1) {
                // wild has not - only if a '*' at end
                c1 = wild[k];
                if (c1 == '*') {
                    // a match if this is the last
                    k++;
                    while (k < len1) {
                        c1 = wild[k];
                        if (c1 == '*') {
                            k++;
                        }
                        else {
                            return 0; // no match
                        }
                    }
                }
            }

        }
        else if (c1 == '?') {
            j++;    // advance name 1
            if (j < len2)
                continue;
            // ok to contine
        }
        else if (c1 == '*') {
            // matches 0 or more chars, 
            // up until the next NON wild
            k = i + 1;
            while (k < len1) {
                c1 = wild[k];
                if (c1 == '*') {
                    k++;
                    continue;
                }
                if (c1 == '?') {
                    // this would technically be an error, but
                    k++;    // allow for now
                    continue;
                }
                // so we have a REAL char, after an '*'
                // this must be found in the name
                if (c1 == c2) {
                    // eaten the wild chars
                    i = k - 1;
                    break;
                }
                break;
            }
            if (k >= len1) {
                // ran out of WILD chars 
                // so this should match all the rest of name
                return 1;
            }
            i = k - 1;
            // MUST find this real char in the name, else
            j++;
            while (j < len2) {
                c2 = name[j];
                if (c1 == c2) {
                    break;
                }
                j++; // use up anothe name char, to match '*A' in wild
            }
            if (c1 == c2) {
                continue;
            }
        }
        if ((k < len1) || (j < len2)) {
            // current chars do NOT match,
            // and are NOT wild, so
            return 0;   // failed in the matching
        }

    }
    if ((i < len1) || (j < len2))
        return 0; // does not match
    return 1; // ran out of WILD and NAME, so is a match
}


int exp_wild(string& path, string& wild, vSTG &m, DiskType type)
{
    int found = 0;
    const char* dir = path.c_str();
    DIR* dp = opendir(dir);
    if (!dp) {
        printf("%s: unable to open directory '%s'\n", module, dir);
        return 0;
    }
    std::string s(dir);
    std::string ff;
    struct dirent* pe;
    fix_directory(s);
    DiskType typ;
    while ((pe = readdir(dp)) != 0) {
        if (strcmp(pe->d_name, ".") == 0)
            continue;
        if (strcmp(pe->d_name, "..") == 0)
            continue;
        ff = s;
        ff += pe->d_name;
        typ = is_file_or_directory32(ff.c_str());
        if (is_match(pe->d_name, wild.c_str())) {
            // this is a match
            if ((typ == MDT_DIR) && (type & MDT_DIR)) {
                m.push_back(pe->d_name);
                found++;
            }
            else if ((typ == MDT_FILE) && (type & MDT_FILE)) {
                m.push_back(pe->d_name);
                found++;
            }

        }
    }
    return found;
}

vSTG split_per_seps(const std::string& str, const char* seps)
{
    vSTG result;
    size_t pos = 0;
    size_t startPos = str.find_first_not_of(seps, 0);
    for (;;)
    {
        pos = str.find_first_of(seps, startPos);
        if (pos == string::npos) {
            result.push_back(str.substr(startPos));
            break;
        }
        result.push_back(str.substr(startPos, pos - startPos));
        startPos = str.find_first_not_of(seps, pos);
        if (startPos == string::npos) {
            break;
        }
    }
    return result;
}

// from : https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c
bool hasEnding(std::string const& fullString, std::string const& ending) 
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    }
    else {
        return false;
    }
}


// could be something like build*/*.c
// but first just deal with build/*.c
// convert input 'path/' to 'path/*',
// and 'path/*.*' to 'path/*'
int expand_wild(char* arg)
{
    int count = 0;
    const char* sep = "\\/";
    //if (VERB9) 
        ::printf("Exapanding Wild: '%s'\n", arg);

    string s, path, p;
    DiskType typ, rtyp;
    size_t i, len;
    size_t j, max;
    size_t k, len2;
    p = arg;    // put it in a atring
    if (hasEnding(p, "\\") || hasEnding(p, "/")) {
        p += "*";   // just add 1 wild card
    }
    else if (hasEnding(p, "*.*")) {
        //p.erase(s.begin() + (p.size() - 2)); // remove the '.*'
        p = p.substr(0, p.size() - 2);
    }
    // else in ALL other cases the LAST token is the FILE name

    vSTG split = split_per_seps(p, sep);
    len = split.size();
    if (!len)
        return 0;

    vSTG paths;
    vSTG matches;
    for (i = 0; i < len; i++) {
        s = split[i];
        if ((i == 0) && (s.size() == 2) && (s[1] == ':')) {
            // ASSUME is a valid DRIVE
            paths.push_back(s);
            continue;
        }
        rtyp = ((i + 1) >= len) ? MDT_FILE : MDT_DIR;
        if (paths.size() == 0)
            paths.push_back("");
        max = paths.size();
        for (j = 0; j < max; j++) {
            path = paths[j];
            if (has_wild((char*)s.c_str())) {
                matches.clear();
                if (exp_wild(path, s, matches, rtyp)) {
                    len2 = matches.size();
                    for (k = 0; k < len2; k++) {
                        p = path;
                        s = matches[k];
                        if (p.size())
                            p += PATH_SEP;
                        p += s;
                        typ = is_file_or_directory32(p.c_str());
                        if (typ == MDT_NONE) {
                            ::printf("Path/file '%s' FAILED!\n", p.c_str());
                            //return 0;
                        }
                        else if (typ == MDT_FILE) {
                            vFiles.push_back(p);
                            count++;
                            if (VERB5) ::printf("%d: Found '%s', in '%s'\n", count, s.c_str(), path.c_str());
                        }
                        else {
                            // is a dir
                            if (k == 0) {
                                paths[j] = p;
                                if (VERB9) ::printf("Modified '%s' to '%s'\n", path.c_str(), p.c_str());
                            } 
                            else {
                                paths.push_back(p);
                                if (VERB9) ::printf("Added path '%s'\n", p.c_str());
                            }
                        }

                    }

                }
                else {
                    ::printf("No match for '%s', in folder '%s' - failed!\n", s.c_str(), path.c_str());
                    //return 0;
                }
            }
            else {
                if (path.size())
                    path += PATH_SEP;
                path += s;
                typ = is_file_or_directory32(path.c_str());
                if (typ == MDT_NONE) {
                    ::printf("Path/file '%s' FAILED!\n", path.c_str());
                    //return 0;
                }
                else if (typ == MDT_FILE) {
                    vFiles.push_back(path);
                    count++;
                    if (VERB5) ::printf("%d: Found '%s'\n", count, path.c_str());
                }
                else {
                    paths[j] = path;
                    if (VERB9) ::printf("Modified path '%s'!\n", path.c_str());
                }
            }
        }
    }
    return count;
}


int expand_wild_nok(char* arg)
{
    int count = 0;
    char* chrs = "\\/";
    vSTG vp;
    std::string s;
    //size_t i, len;
    //char* res = strchr(arg, '\\');
    // printf("Exapanding Wild: '%s'\n", arg);
    char* p2 = strpbrk(arg, chrs);
    char* tmp = 0;
    char* pch = 0;
    char* tok = strtok(arg, chrs);
    if (tok) {
        vp.push_back(tok);
        while ((tmp = strtok(NULL, chrs)) != NULL) {
            pch = tmp;
            vp.push_back(tmp);
        }
    }
    return count;
}
#endif //HAVE_DIRENT_H


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
                printf("%s: Unknown argument '%s'. Try -? for help...\n", module, arg);
                return 1;
            }
        } else {
            // bear argument
#ifdef HAVE_DIRENT_H
            if (has_wild(arg)) {
                if (!expand_wild(arg)) {
                    printf("%s: No files found in path %s!\n", module, arg);
                }
            }
            else {
                DiskType dt = is_file_or_directory32(arg);
                if (dt == MDT_FILE)
                    vFiles.push_back(arg);
                else if (dt == MDT_DIR) {
                    if (!expand_dir(arg)) {
                        printf("%s: No files found in dir %s!\n", module, arg);
                    }
                }
            }



#else // !#ifdef HAVE_DIRENT_H
            if (usr_input) {
                printf("%s: Already have input '%s'! What is this '%s'?\n", module, usr_input, arg );
                return 1;
            }
            if (is_file_or_directory32(arg) != MDT_FILE) {
                printf("%s: Unable to 'stat' file '%s'!\n", module, arg );
                return 1;
            }
#endif // #ifdef HAVE_DIRENT_H y/n
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

#ifdef HAVE_DIRENT_H
    for (auto s : vFiles) {
        usr_input = s.c_str();

        if (valid_utf8_file(usr_input)) {
            if (VERB1) {
                printf("%s: File '%s' appears to contain valid UTF-8 text\n", module, usr_input);
            }
        }
        else {
            if (VERB1) {
                printf("\n%s: File '%s' appears to contain INVALID UTF-8 text\n\n", module, usr_input);
            }
            iret = 1;
            // do_full_test = true;
        }

        if (do_full_test) {
            iret |= utf8_test(usr_input);
        }
    }
#else //#ifdef HAVE_DIRENT_H
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

#endif // #ifdef HAVE_DIRENT_H
    return iret;
}


// eof = utf8-test.cxx
