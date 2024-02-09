

#ifndef _bom_h_
#define _bom_h_


#include "../tiny-json/tiny-json.h"

typedef struct {
	void *source;
	void *spool;
	int freeSource;
	
	json_t const *root;
	json_t const *json;
	
	json_t const *volume;
	json_t const *book;
	json_t const *chapter;
	json_t const *verse;
}js_t;

typedef struct {
    json_t **mem;
    unsigned int nextFree;
    unsigned int totalMemPools;
	unsigned int totalAlloc;
	unsigned int currentPool;
    jsonPool_t pool;
}jsonStaticSpool_t;

typedef void (*jsonObjCB_t) (json_t const *json, js_t *hJs);

#ifdef __cplusplus
extern "C" {
#endif

js_t *js_importJsonPath (const char *path);
js_t *js_importJsonBuffer (void *buffer, const size_t length);

void js_free (js_t *hJs);

void js_versePrint (js_t *hJs);
json_t const *js_openVerse (js_t *hJs, const uint32_t verseNo);
json_t const *js_openChapter (js_t *hJs, const uint32_t chapterNo);
char const *js_bookHeading (js_t *hJs);
char const *js_bookTitle (js_t *hJs);
json_t const *js_openBook (js_t *hJs, const char *book);
char const *js_volumeTitle (js_t *hJs);
json_t const *js_openVolumes (js_t *hJs, const char *volume);

void js_closeBook (js_t *hJs);
void js_closeVerse (js_t *hJs);
void js_closeChapter (js_t *hJs);
void js_verseCopy (js_t *hJs, char *buffer, uint32_t bufferLen);
char *js_verseDup (js_t *hJs);
char const *js_verseGet (js_t *hJs);
uint32_t js_verseLength (js_t *hJs);

#ifdef __cplusplus
}
#endif


#endif
