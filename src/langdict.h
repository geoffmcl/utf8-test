/* langdict.h */
#ifndef _LANGDICT_H_
#define _LANGDIST_H_

//typedef struct languageDictionaryEntry {
typedef struct tagLDEntry {
	int key;
	int pluralForm;
	char *value;
} LDEntry;

//typedef languageDictionaryEntry const languageDictionary[600];
typedef LDEntry const LDict[600];

typedef struct tagLDDef {
	int (*whichPluralForm)(int n);
	LDict messages;
} LDDef;

static int whichPluralForm_zh_cn(int n) {
	/* Plural-Forms: nplurals=1; */
	return 0;
}

typedef enum
{
	/* This MUST be present and first. */
	TIDY_MESSAGE_TYPE_FIRST = 4096,
	
	/* Specify the code for this language. */
	TIDY_LANGUAGE,

	TC_STRING_VERS_A,

	/* This MUST be present and last. */
	TIDY_MESSAGE_TYPE_LAST
} tidyMessageTypes;

typedef enum {
    ACCESS_URL = 2048,          /* Used to point to Web Accessibility Guidelines. */
    ATRC_ACCESS_URL,            /* Points to Tidy's accessibility page. */
    FILE_CANT_OPEN,             /* For retrieving a string when a file can't be opened. */
    LINE_COLUMN_STRING,         /* For retrieving localized `line %d column %d` text. */
    // ...
    STRING_CONTENT_LOOKS,       /* `Document content looks like %s`. */

    // ...
    MISC_MAX_VALUE
} tidyMessagesMisc;

#ifdef __cplusplus
extern "C" {
#endif

extern char *get_en_stg(int key);
extern char *get_zh_stg(int key);

#ifdef __cplusplus
}
#endif


#endif // _LANGDICT_H_
/* eof */
