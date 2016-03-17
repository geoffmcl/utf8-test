/*\
 * chk-BOM.cxx
 *
 * Copyright (c) 2015 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/

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

#define DEF_IN_FILE "F:\\Projects\\utf8\\src\\langdict.cxx"
#ifndef SPRTF
#define SPRTF printf
#endif

static const char *module = "chk-BOM";

static const char *usr_input = 0;
typedef std::vector<std::string> vSTG;

static vSTG vBoms;
void give_help( char *name )
{
    printf("%s: usage: [options] usr_input\n", module);
    printf("Options:\n");
    printf(" --help  (-h or -?) = This help and exit(0)\n");
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


int process_file( const char *file ) 
{
    int iret = 0;
    size_t len;
    unsigned char buf[8];
    memset(buf,0,8);
    FILE *fp = fopen(file,"rb");
    if (fp) {
        len = fread(buf,1,8,fp);
        fclose(fp);
        printf("%s: Reading file '%s' - %d bytes...\n", module, file, len);
        if (len > 0) {
            printf("Hex:%d: ", len);
            show_hexified((char *)buf, len, 0);
        }
        if ((len >= 3) && ( buf[0] == 0xef ) && (buf[1] == 0xbb) && (buf[2] == 0xbf)) {
            printf("%s: Has a UTF-8 BOM\n", module);
            iret = 1;
        } else if ((len >= 2) && ( buf[0] == 0xfe ) && (buf[1] == 0xff) ) {
            printf("%s: Has a UTF-16 (BE) BOM\n", module);
            iret = 1;
        } else if ((len >= 2) && ( buf[0] == 0xff ) && (buf[1] == 0xfe) ) {
            printf("%s: Has a UTF-16 (LE) BOM\n", module);
            iret = 1;
        } else if ((len >= 4) && ( buf[0] == 0) && (buf[1] == 0) && (buf[2] == 0xfe) && (buf[3] == 0xff)) {
            printf("%s: Has a UTF-32 (BE) BOM\n", module);
            iret = 1;
        } else if ((len >= 4) && ( buf[0] == 0xff) && (buf[1] == 0xfe) && (buf[2] == 0) && (buf[3] == 0)) {
            printf("%s: Has a UTF-32 (LE) BOM\n", module);
            iret = 1;
        } else if ((len >= 4) && ( buf[0] == 0x2b) && (buf[1] == 0x2f) && (buf[2] == 0x76) &&
            ((buf[3] == 0x38) || (buf[3] == 0x39) || (buf[3] == 0x2B) || (buf[3] == 0x2f))   ) {
            // # UTF-7       2B 2F 76, and one of: [38|39|2B|2F] 43 47 118, and one of: [56|57|43|47] +/v, and one of 8 9 + / 
            printf("%s: Has a UTF-7 BOM\n", module);
            iret = 1;
        } else if ((len >= 3) && ( buf[0] == 0xf7 ) && (buf[1] == 0x64) && (buf[2] == 0x4c)) {
            printf("%s: Has a UTF-1 BOM\n", module);
            iret = 1;
        } else if ((len >= 4) && ( buf[0] == 0xdd) && (buf[1] == 0x73) && (buf[2] == 0x66) && (buf[3] == 0x73)) {
            printf("%s: Has a UTF-EBCDIC BOM\n", module );
            iret = 1;
        } else if ((len >= 3) && ( buf[0] == 0x0e ) && (buf[1] == 0xfe) && (buf[2] == 0xff)) {
            printf("%s: Has a SCSU BOM\n", module);
            iret = 1;
        } else if ((len >= 3) && ( buf[0] == 0xfb ) && (buf[1] == 0xee) && (buf[2] == 0x28)) {
            printf("%s: Has a BOCU-1 BOM\n", module);
            iret = 1;
        } else if ((len >= 4) && ( buf[0] == 0x84) && (buf[1] == 0x31) && (buf[2] == 0x95) && (buf[3] == 0x33)) {
            printf("%s: Has a GB-18030 BOM\n", module );
            iret = 1;
        } else {
            printf("%s: Does not appear to have a BOM\n", module);
        }
    } else {
        printf("%s: Failed to open '%s'!\n", module, file);
        iret = 2;
    }
    if (iret == 1) {
        vBoms.push_back(file);
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
                    iret |= process_file( file.c_str() ); 
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
        iret = process_file(usr_input);
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
    size_t ii, max = vBoms.size();
    printf("%s: Found %d file(s) with BOM\n", module, (int)max );
    for (ii = 0; ii < max; ii++) {
        s = vBoms[ii];
        printf("%s\n", s.c_str());
    }
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
    printf("%s: exit(%d)\n", module, iret );
    return iret;
}


// eof = chk-BOM.cxx
