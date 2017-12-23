/*\
 * chk-BOM.cxx
 *
 * Copyright (c) 2015-2017 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
 * 20171223 - Add verbosity
\*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> // for strdup(), ...
#include <string>
#include <vector>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#include "langdict.h"

// other includes
#ifndef ISDIGIT
#define ISDIGIT(a) ( ( a >= '0' ) && ( a <= '9' ) )
#endif

#define DEF_IN_FILE "F:\\Projects\\utf8\\src\\langdict.cxx"
#ifndef SPRTF
#define SPRTF printf
#endif

static const char *module = "chk-BOM";
/*
# BOM Type    Hex          = Decimal
# UTF-8       EF BB BF     = 239 187 191        //  0xEF,0xBB,0xBF
# UTF-16 (BE) FE FF        = 254 255
# UTF-16 (LE) FF FE        = 255 254
# UTF-32 (BE) 00 00 FE FF  = 0 0 254 255
# UTF-32 (LE) FF FE 00 00  = 255 254 0 0
# UTF-7       2B 2F 76, and one of: [38|39|2B|2F] 43 47 118, and one of: [56|57|43|47] +/v, and one of 8 9 + /
# UTF-1       F7 64 4C     = 247 100 76
# UTF-EBCDIC  DD 73 66 73  = 221 115 102 115
# SCSU        0E FE FF     = 14 254 255
# BOCU-1      FB EE 28 +optional FF 251 238 40 +optional 255
# GB-18030    84 31 95 33  = 132 49 149 51
*/

enum bomtype {
    nobom,
    utf8,
    utf16be,
    utf16le,
    utf32be,
    utf32le,
    utf7,
    utf1,
    utfeb,
    scsu,
    bocu,
    gb18
};

typedef std::vector<bomtype> vBT;


typedef struct tagBomList {
    bomtype type;
    const char *name;
    const char *hex;
    const char *dec;
}BomList, *PBomList;


//# BOM Type    Hex = Decimal
BomList bom_list[] = {
    { utf8, "UTF-8", "EF BB BF", "239 187 191" },        //  0xEF,0xBB,0xBF
    { utf16be, "UTF-16 (BE)", "FE FF", "254 255" },
    { utf16le, "UTF-16 (LE)", "FF FE", "255 254" },
    { utf32be, "UTF-32 (BE)", "00 00 FE FF", "0 0 254 255" },
    { utf32le, "UTF-32 (LE)", "FF FE 00 00", "255 254 0 0" },
    { utf7, "UTF-7", "2B 2F 76 [38|39|2B|2F]", "43 47 118 [56|57|43|47]" }, // +/ v, and one of 8 9 + /
    { utf1, "UTF-1", "F7 64 4C", "247 100 76" },
    { utfeb, "UTF-EBCDIC", "DD 73 66 73", "221 115 102 115" },
    { scsu, "SCSU", "0E FE FF", "14 254 255" },
    { bocu, "BOCU-1", "FB EE 28 +o FF", "251 238 40 +o 255" },
    { gb18, "GB-18030", "84 31 95 33", "132 49 149 51" },
    // last
    { nobom, 0, 0, 0 }
};

const char *get_bt(bomtype bt)
{
    PBomList pb = &bom_list[0];
    while (pb->name) {
        if (pb->type == bt)
            return pb->name;
        pb++;
    }
    return "N/A";

}

static int verbosity = 0;
static const char *usr_input = 0;
typedef std::vector<std::string> vSTG;

static vSTG vBoms;
static vBT vTypes;

#define VERB1 ( verbosity >= 1 )
#define VERB2 ( verbosity >= 2 )
#define VERB5 ( verbosity >= 5 )
#define VERB9 ( verbosity >= 9 )

void give_help( char *name )
{
    printf("%s: usage: [options] usr_input\n", module);
    printf("Options:\n");
    printf(" --help  (-h or -?) = This help and exit(0)\n");
    printf(" --verb[n]     (-v) = Bump or set verbosity. n=0,1,2,5,9 (def=%d)\n", verbosity);
    // TODO: More help
    printf("\n");
    printf(" Given an input file, open and read initial bytes, seeking\n");
    printf(" a known Byte Order Mark (BOM), and try to name the BOM,\n");
    printf(" and exit(1) if one found, else exit(0). Exit(2) if file read fails.\n");
#ifdef HAVE_DIRENT_H
    printf(" Given an input directory, search all files in that directory\n");
#endif
    printf("\n");

}

#ifdef _MSC_VER
#define M_IS_DIR _S_IFDIR
#else // !_MSC_VER
#define M_IS_DIR S_IFDIR
#endif
#ifdef WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

enum DiskType {
    MDT_NONE,
    MDT_FILE,
    MDT_DIR
};


static struct stat buf;
DiskType is_file_or_directory ( const char * path )
{
    if (!path)
        return MDT_NONE;
	if (stat(path,&buf) == 0)
	{
		if (buf.st_mode & M_IS_DIR)
			return MDT_DIR;
		else
			return MDT_FILE;
	}
	return MDT_NONE;
}

size_t get_last_file_size() { return buf.st_size; }


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
                if (VERB9)
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
#ifndef NDEBUG
    if (!usr_input) {
        usr_input = strdup(DEF_IN_FILE);
    }
#endif
    if (!usr_input) {
        printf("%s: No user input found in command!\n", module);
        return 1;
    }
    return 0;
}

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


int process_file( const char *file, size_t size ) 
{
    int iret = 0;
    size_t len;
    unsigned char buf[8];
    bomtype bt = nobom;
    memset(buf,0,sizeof(buf));
    FILE *fp = fopen(file,"rb");
    if (fp) {
        len = fread(buf,1,8,fp);
        fclose(fp);
        if (VERB2)
            printf("%s: Reading file '%s' - %d bytes, of total %d...\n", module, file, (int)len, (int)size);
        if ((len > 0) && VERB5) {
            printf("Hex:%d: ", (int)len);
            show_hexified((char *)buf, (int)len, 0);
        }
        if ((len >= 3) && ( buf[0] == 0xef ) && (buf[1] == 0xbb) && (buf[2] == 0xbf)) {
            bt = utf8;
            if (VERB2) printf("%s: Has a UTF-8 BOM\n", module);
            iret = 1;
        } else if ((len >= 2) && ( buf[0] == 0xfe ) && (buf[1] == 0xff) ) {
            bt = utf16be;
            if (VERB2) printf("%s: Has a UTF-16 (BE) BOM\n", module);
            iret = 1;
        } else if ((len >= 2) && ( buf[0] == 0xff ) && (buf[1] == 0xfe) ) {
            bt = utf16le;
            if (VERB2) printf("%s: Has a UTF-16 (LE) BOM\n", module);
            iret = 1;
        } else if ((len >= 4) && ( buf[0] == 0) && (buf[1] == 0) && (buf[2] == 0xfe) && (buf[3] == 0xff)) {
            bt = utf32be;
            if (VERB2) printf("%s: Has a UTF-32 (BE) BOM\n", module);
            iret = 1;
        } else if ((len >= 4) && ( buf[0] == 0xff) && (buf[1] == 0xfe) && (buf[2] == 0) && (buf[3] == 0)) {
            bt = utf32le;
            if (VERB2) printf("%s: Has a UTF-32 (LE) BOM\n", module);
            iret = 1;
        } else if ((len >= 4) && ( buf[0] == 0x2b) && (buf[1] == 0x2f) && (buf[2] == 0x76) &&
            ((buf[3] == 0x38) || (buf[3] == 0x39) || (buf[3] == 0x2B) || (buf[3] == 0x2f))   ) {
            // # UTF-7       2B 2F 76, and one of: [38|39|2B|2F] 43 47 118, and one of: [56|57|43|47] +/v, and one of 8 9 + / 
            bt = utf7;
            if (VERB2) printf("%s: Has a UTF-7 BOM\n", module);
            iret = 1;
        } else if ((len >= 3) && ( buf[0] == 0xf7 ) && (buf[1] == 0x64) && (buf[2] == 0x4c)) {
            bt = utf1;
            if (VERB2) printf("%s: Has a UTF-1 BOM\n", module);
            iret = 1;
        } else if ((len >= 4) && ( buf[0] == 0xdd) && (buf[1] == 0x73) && (buf[2] == 0x66) && (buf[3] == 0x73)) {
            bt = utfeb;
            if (VERB2) printf("%s: Has a UTF-EBCDIC BOM\n", module );
            iret = 1;
        } else if ((len >= 3) && ( buf[0] == 0x0e ) && (buf[1] == 0xfe) && (buf[2] == 0xff)) {
            bt = scsu;
            if (VERB2) printf("%s: Has a SCSU BOM\n", module);
            iret = 1;
        } else if ((len >= 3) && ( buf[0] == 0xfb ) && (buf[1] == 0xee) && (buf[2] == 0x28)) {
            bt = bocu;
            if (VERB2) printf("%s: Has a BOCU-1 BOM\n", module);
            iret = 1;
        } else if ((len >= 4) && ( buf[0] == 0x84) && (buf[1] == 0x31) && (buf[2] == 0x95) && (buf[3] == 0x33)) {
            bt = gb18;
            if (VERB2) printf("%s: Has a GB-18030 BOM\n", module );
            iret = 1;
        } else {
            if (VERB5) printf("%s: Does not appear to have a BOM\n", module);
        }
    } else {
        printf("%s: Failed to open '%s'!\n", module, file);
        iret = 2;
    }
    if (bt != nobom) {
        vBoms.push_back(file);
        vTypes.push_back(bt);
    }
    return iret;
}

#ifdef HAVE_DIRENT_H

int process_directory( const char *dir )
{
    int iret = 0;
    DIR *dp = opendir(dir);
    if (dp) {
        std::string file, bdir = dir;
        struct dirent *pe = readdir(dp);
        bdir += PATH_SEP;
        while (pe) {
            if (strcmp(pe->d_name,".") && strcmp(pe->d_name,"..")) {
                file = bdir;
                file += pe->d_name;
                if (is_file_or_directory(file.c_str()) == MDT_FILE) {
                    iret |= process_file( file.c_str(), get_last_file_size() ); 
                }
            }
            pe = readdir(dp);
        }
        closedir(dp);
    } else {
        printf("%s: Failed to open directory '%s'!\n", module, dir );
        iret = 2;
    }
    return iret;
}
#endif // #ifdef HAVE_DIRENT_H


int check_input()
{
    int iret = 0;
    if (!usr_input) {
        printf("%s: No input given!\n", module);
        return 2;
    }
    DiskType dt = is_file_or_directory(usr_input);
    if (dt == MDT_FILE) {
        iret = process_file(usr_input, get_last_file_size() );
    } else {
#ifdef HAVE_DIRENT_H
        if (dt == MDT_DIR) {
            iret = process_directory( usr_input );
        } else {
            printf("%s: Input '%s' is NOT a file nor directory!\n", module, usr_input);
            iret = 2;
        }
#else
        printf("%s: Input '%s' is NOT a file!\n", module, usr_input);
        iret = 2;
#endif

    }

    return iret;
}

size_t show_boms()
{
    std::string s;
    bomtype bt = nobom;
    size_t ii, max = vBoms.size();
    printf("%s: Found %d file(s) with BOM\n", module, (int)max );
    for (ii = 0; ii < max; ii++) {
        s = vBoms[ii];
        bt = vTypes[ii];
        printf("%s", s.c_str());
        if (VERB1) {
            const char *cp = get_bt(bt);
            printf(" - %s", cp);
        }
        printf("\n");
    }
    if (VERB5)
        printf("%s: Shown %d files...\n", module, (int)ii);
    vBoms.clear();
    return max;
}

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

    iret = check_input(); // actions of app
    show_boms();
    if (VERB5)
        printf("%s: exit(%d)\n", module, iret );
    return iret;
}


// eof = chk-BOM.cxx
