/*\
 * langdict.cxx
 *
 * Copyright (c) 2015 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h> // for strdup(), ...
#include "langdict.h"

#ifdef _MSC_VER
// http://cboard.cprogramming.com/cplusplus-programming/145590-non-english-characters-cout-2.html#post1086757
#pragma execution_character_set("utf-8") 
#endif

// other includes

static const char *module = "langdict";

static char *chin = "s.Chin: 汉字; t.Chin: 漢字";
static char *Chin = "s.chin: \xe6\xb1\x89\xe5\xad\x97;";

//static languageDefinition language_zh_cn = { whichPluralForm_zh_cn, {
static LDDef language_zh_cn = { whichPluralForm_zh_cn, {
    { TIDY_LANGUAGE,          0, "zh_cn"               }, /* Specify the ll or ll_cc language code here. */
    { FILE_CANT_OPEN,         0, "无法打开"            },
    { LINE_COLUMN_STRING,     0, "行 列"               },
    { STRING_CONTENT_LOOKS,   0, "文档内容看起来像"     },
    {/* The strings "Tidy" and "HTML Tidy" are the program name and must not be translated. */
      TC_STRING_VERS_A,       0, "HTML Tidy 版本 用于"
    },

    /* This MUST be present and last. */
    { TIDY_MESSAGE_TYPE_LAST, 0, NULL                           }

}};

static LDDef language_en = { whichPluralForm_zh_cn, {
    { TIDY_LANGUAGE,          0, "en"                      }, /* Specify the ll or ll_cc language code here. */

    { FILE_CANT_OPEN,         0, "Can't open"              },
    { LINE_COLUMN_STRING,     0, "Row Col"                 },
    { STRING_CONTENT_LOOKS,   0, "The contents look like " },
    { TC_STRING_VERS_A,       0, "HTML Tidy for version "  },

    /* This MUST be present and last. */
    { TIDY_MESSAGE_TYPE_LAST, 0, NULL                      }

}};

char *get_en_stg(int key)
{
    int i, t;
    char *cp;
    for (i = 0; ; i++) {
        t = language_en.messages[i].key;
        cp = language_en.messages[i].value;
        if (t == TIDY_MESSAGE_TYPE_LAST)
            break;
        if (t == key )
            return cp;
    }
    return NULL;
}

char *get_zh_stg(int key)
{
    int i, t;
    char *cp;
    for (i = 0; ; i++) {
        t = language_zh_cn.messages[i].key;
        cp = language_zh_cn.messages[i].value;
        if (t == TIDY_MESSAGE_TYPE_LAST)
            break;
        if (t == key )
            return cp;
    }
    return NULL;
}

/* eof */
