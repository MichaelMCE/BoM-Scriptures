
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <conio.h>
#include <inttypes.h>

#include "../tiny-json/tiny-json.h"
#include "bom.h"



static json_t *bom_poolInit (jsonPool_t *pool)
{
    jsonStaticSpool_t *spool = json_containerOf(pool, jsonStaticSpool_t, pool);
    spool->nextFree = 1;
	spool->currentPool = 0;
	spool->totalMemPools = 1;
	spool->totalAlloc = 128;		// total json_t in each pool
	
	spool->mem = extmem_calloc(spool->totalMemPools, sizeof(json_t*));
	spool->mem[0] = extmem_calloc(spool->totalAlloc, sizeof(json_t));
    return &spool->mem[spool->currentPool][0];
}

static json_t *bom_poolAlloc (jsonPool_t *pool)
{
    jsonStaticSpool_t *spool = json_containerOf(pool, jsonStaticSpool_t, pool);
    
	if (!spool) return NULL;

    if (spool->nextFree >= spool->totalAlloc){
    	spool->totalAlloc += 4096;
		spool->totalMemPools++;
		spool->mem = extmem_realloc(spool->mem, spool->totalMemPools  *sizeof(json_t*));
		spool->mem[++spool->currentPool] = extmem_calloc(spool->totalAlloc, sizeof(json_t));
    }

	void *ptr = &spool->mem[spool->currentPool][spool->nextFree++];
    return ptr; 
}

static void bom_poolFree (jsonPool_t *pool)
{
	jsonStaticSpool_t *spool = json_containerOf(pool, jsonStaticSpool_t, pool);
	
	for (int i = 0; i < spool->totalMemPools; i++)
		extmem_free(spool->mem[i]);

	extmem_free(spool->mem);
}

void js_free (js_t *hJs)
{	
	bom_poolFree(&((jsonStaticSpool_t*)hJs->spool)->pool);
    free(hJs->spool);
    if (hJs->freeSource == 1)
    	free(hJs->source);
}

static size_t fileLength (FILE *fp)
{
	fpos_t pos;
	
	fgetpos(fp, &pos);
	fseek(fp, 0, SEEK_END);
	size_t fl = ftell(fp);
	fsetpos(fp, &pos);
	
	return fl;
}

void *js_loadFile (const char *filename, size_t *flength)
{

	*flength = 0;
	void *data = NULL;
	FILE *fd = fopen(filename, "rb");
	
	if (fd){
		*flength = fileLength(fd);
		if (*flength > 63){
			data = malloc(*flength);
			if (data){
				size_t ret = (size_t)fread(data, 1, *flength, fd);
				if (ret != *flength){
					printf("getfile(): file read error\n");
					free(data);
					data = NULL;
					*flength = 0;
				}
			}
		}
		fclose(fd);
	}
	
	return data;
}

js_t *js_importJsonBuffer (void *buffer, const size_t length)
{
    jsonStaticSpool_t spool = {.pool = {.init = bom_poolInit, .alloc = bom_poolAlloc}};
    json_t const *json = json_createWithPool(buffer, &spool.pool);
    if (!json) {
        printf("js_importJsonBuffer(): error json create\n");
        return NULL;
    }

	js_t *hJs = calloc(1, sizeof(js_t));
	if (hJs){
		hJs->json = json;
		hJs->root = json;
		hJs->source = buffer;
		hJs->freeSource = 0;

		hJs->spool = malloc(sizeof(spool));
		if (!hJs->spool){
			free(hJs);
			return NULL;
		}
		memcpy(hJs->spool, &spool, sizeof(spool));
	}
	return hJs;
}

js_t *js_importJsonPath (const char *path)
{

	size_t length = 0;
	char *str = js_loadFile(path, &length);
	if (!str){
		printf("js_loadJson(): Load failed\n");
		return NULL;
	}

	js_t *hJs = js_importJsonBuffer(str, length);
	if (hJs){
		hJs->freeSource = 1;
	}else {
		free(str);
	}
	return hJs;
}

void js_processType (json_t const *json, const jsonType_t propertyType)
{
	char const *name = json_getName(json);
	if (name){
		if (propertyType == JSON_OBJ) printf(" JSON_OBJ: \"%s\"\n", name);
		if (propertyType == JSON_ARRAY) printf(" JSON_ARRAY: \"%s\"\n", name);
		if (propertyType == JSON_TEXT) printf(" JSON_TEXT: \"%s\"\n", name);
		if (propertyType == JSON_BOOLEAN) printf(" JSON_BOOLEAN: \"%s\"\n", name);
		if (propertyType == JSON_INTEGER) printf(" JSON_INTEGER: \"%s\"\n", name);
		if (propertyType == JSON_REAL) printf(" JSON_REAL: \"%s\"\n", name);
		if (propertyType == JSON_NULL) printf(" JSON_NULL: \"%s\"\n", name);
	}
}

#if 0
void js_dump (json_t const *json, js_t *hJs)
{

    jsonType_t type = json_getType(json);
    if (type != JSON_OBJ && type != JSON_ARRAY){
        puts("error");
        return;
    }

    for (; json != NULL; json = json_getSibling(json)){
    	jsonType_t propertyType = json_getType(json);
		js_processType(json, propertyType);

		if (propertyType == JSON_TEXT){
			char const *value = json_getValue(json);
			if (value) printf("\t\t'%s'\n", value);
						
		}else if (propertyType == JSON_INTEGER){
			int32_t value = (int32_t)json_getInteger(json);
			if (value) printf("\t\t%i\n", (int)value);

		}else if (propertyType == JSON_OBJ){
			json_t const *jsonObj = json_getChild(json);

			for (jsonObj = json_getChild(json); jsonObj != NULL; jsonObj = json_getSibling(jsonObj)){
				propertyType = json_getType(jsonObj);
				js_processType(jsonObj, propertyType);

				if (propertyType == JSON_OBJ){
					json_t const *child = json_getChild(jsonObj);
					
					for (child = json_getChild(jsonObj); child != NULL; child = json_getSibling(child)){
						propertyType = json_getType(child);
						js_processType(child, propertyType);
						
						if (propertyType == JSON_TEXT){
							char const *value = json_getValue(child);
							if (value) printf("\t\t'%s'\n", value);
						}
					}
				}else if (propertyType == JSON_TEXT){
					char const *value = json_getValue(jsonObj);
					if (value) printf("\t\t'%s'\n", value);
							
				}else if (propertyType == JSON_INTEGER){
					int32_t value = (int32_t)json_getInteger(jsonObj);
					if (value) printf("\t\t%i\n", (int)value);
					
				}else if (propertyType == JSON_ARRAY){
					json_t const *array;
					for (array = json_getChild(jsonObj); array != NULL; array = json_getSibling(array)){
						jsonType_t propertyType = json_getType(array);
						js_processType(array, propertyType);
						
						if (propertyType == JSON_TEXT){
							char const *value = json_getValue(array);
							if (value) printf("\t\t'%s'\n", value);
						}
					}
				}
			}
		}else if (propertyType == JSON_ARRAY){
		   	json_t const *child;
		
			// process book
			for (child = json_getChild(json); child != NULL; child = json_getSibling(child)){
				jsonType_t propertyType = json_getType(child);
				js_processType(child, propertyType);
	
				if (propertyType == JSON_TEXT){
					char const *value = json_getValue(child);
					if (value) printf("\t\t'%s'\n", value);
							
				}else if (propertyType == JSON_INTEGER){
					int32_t value = (int32_t)json_getInteger(child);
					if (value) printf("\t\t%i\n", (int)value);

				}else if (propertyType == JSON_OBJ){
					json_t const *childObj = json_getChild(child);
					
					// process book objects
					for (childObj = json_getChild(child); childObj != NULL; childObj = json_getSibling(childObj)){
						propertyType = json_getType(childObj);
						js_processType(childObj, propertyType);
						
						if (propertyType == JSON_TEXT){
							char const *value = json_getValue(childObj);
							if (value) printf("\t\t'%s'\n", value);
						
						// process chapters	
						}else if (propertyType == JSON_ARRAY){
							json_t const *array;
							
							for (array = json_getChild(childObj); array != NULL; array = json_getSibling(array)){
								propertyType = json_getType(array);
		
								if (propertyType == JSON_TEXT){
									char const *value = json_getValue(array);
									if (value) printf("\t\t'%s'\n", value);
		
								}else if (propertyType == JSON_OBJ){
									json_t const *arrayObj;
		
									// chapter objects
									for (arrayObj = json_getChild(array); arrayObj != NULL; arrayObj = json_getSibling(arrayObj)){
										propertyType = json_getType(arrayObj);
										js_processType(arrayObj, propertyType);
										
										if (propertyType == JSON_TEXT){
											char const *value = json_getValue(arrayObj);
											if (value) printf("\t\t'%s'\n", value);
											
										}else if (propertyType == JSON_INTEGER){
											int32_t value = (int32_t)json_getInteger(arrayObj);
											if (value) printf("\t\t%i\n", (int)value);
											
										// verses
										}else if (propertyType == JSON_ARRAY){
											json_t const *verse;
											for (verse = json_getChild(arrayObj); verse != NULL; verse = json_getSibling(verse)){
												propertyType = json_getType(verse);
												js_processType(verse, propertyType);
												
												// verse objects
												if (propertyType == JSON_OBJ){
													json_t const *verseObj;
													for (verseObj = json_getChild(verse); verseObj != NULL; verseObj = json_getSibling(verseObj)){
														propertyType = json_getType(verseObj);
														js_processType(verseObj, propertyType);
																							
														if (propertyType == JSON_TEXT){
															char const *value = json_getValue(verseObj);
															if (value) printf("\t\t'%s'\n", value);
											
														}else if (propertyType == JSON_INTEGER){
															int32_t value = (int32_t)json_getInteger(verseObj);
															if (value) printf("\t\t%i\n", value);		
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
		    }
		}
	}
}
#endif 

json_t const *js_openVolumes (js_t *hJs, const char *volume)
{
	//printf("js_openVolumes @%s@\n", volume);
	
	json_t const *json;
    for (json = json_getChild(hJs->json); json != NULL; json = json_getSibling(json)){
    	jsonType_t propertyType = json_getType(json);
		//js_processType(json, propertyType);

		if (propertyType == JSON_TEXT){
			char const *name = json_getName(json);
			if (!strcmp("title", name)){
				char const *value = json_getValue(json);
				if (!strcmp(value, volume)){	
					//printf("### found volume\n");					
					//if (value) printf("'%s'\n", value);
					hJs->volume = hJs->json;
					return json;
				}
			}
		}
	}
	
	return NULL;
}

char const *js_volumeTitle (js_t *hJs)
{
	json_t const *json;
    for (json = json_getChild(hJs->volume); json != NULL; json = json_getSibling(json)){
    	jsonType_t propertyType = json_getType(json);
		//js_processType(json, propertyType);

		if (propertyType == JSON_TEXT){
			char const *name = json_getName(json);
			if (!strcmp("title", name)){
				char const *value = json_getValue(json);
				//if (value) printf("'%s'\n", value);
				//return strdup(value);
				return value;
			}
		}
	}
	
	return NULL;
}

json_t const *js_openBook (js_t *hJs, const char *book)
{
	
	//printf("js_openBook @%s@\n", book);

	json_t const *json;
	
    for (json = json_getChild(hJs->json); json != NULL; json = json_getSibling(json)){
    	jsonType_t propertyType = json_getType(json);
		//js_processType(json, propertyType);

		if (propertyType == JSON_ARRAY){
			json_t const *array;
			for (array = json_getChild(json); array != NULL; array = json_getSibling(array)){
				jsonType_t propertyType = json_getType(array);
				//js_processType(array, propertyType);
				
				if (propertyType == JSON_OBJ){
					json_t const *child = json_getChild(array);

					for (child = json_getChild(array); child != NULL; child = json_getSibling(child)){
						jsonType_t propertyType = json_getType(child);
						//js_processType(child, propertyType);

						char const *name = json_getName(child);
						if (!strcmp("book", name)){
							if (propertyType == JSON_TEXT){
								char const *value = json_getValue(child);
								//if (value) printf("\t\t'%s'\n", value);
							
								if (!strcmp(book, value)){
									//printf("@@@ found book #%s#\n", value);
									hJs->book = hJs->json;
									hJs->json = array;
									return array;
								}
							}
						}
					}
				}
			}
		}
	}
	
	return NULL;
}

char const *js_bookTitle (js_t *hJs)
{
	//printf("--- js_bookTitle\n");
	
	json_t const *child;
	
	for (child = json_getChild(hJs->json); child != NULL; child = json_getSibling(child)){
		jsonType_t propertyType = json_getType(child);
		//js_processType(child, propertyType);

		if (propertyType == JSON_TEXT){
			char const *name = json_getName(child);
			//printf("name #%s#\n", name);
			
			if (!strcmp("full_title", name)){
				char const *value = json_getValue(child);
				//if (value) return strdup(value);
				return value;
			}
		}
	}
	return NULL;
}

char const *js_bookHeading (js_t *hJs)
{
	//printf("--- js_bookHeading\n");
	
	json_t const *child;
	
	for (child = json_getChild(hJs->book); child != NULL; child = json_getSibling(child)){
		jsonType_t propertyType = json_getType(child);
		//js_processType(child, propertyType);

		if (propertyType == JSON_TEXT){
			char const *name = json_getName(child);
			//printf("name #%s#\n", name);
			
			if (!strcmp("heading", name)){
				char const *value = json_getValue(child);
				//if (value) return strdup(value);
				return value;
			}
		}
	}
	return NULL;
}

json_t const *js_openChapter (js_t *hJs, const uint32_t chapterNo)
{
	//printf("--- js_openChapter %i\n", chapterNo);
	
	json_t const *child;
	
	for (child = json_getChild(hJs->json); child != NULL; child = json_getSibling(child)){
		jsonType_t propertyType = json_getType(child);
		//js_processType(child, propertyType);

		if (propertyType == JSON_ARRAY){
			char const *name = json_getName(child);
			if (!strcmp("chapters", name)){
				//printf("found chapters \n");
				
				json_t const *array;
				for (array = json_getChild(child); array != NULL; array = json_getSibling(array)){
					jsonType_t propertyType = json_getType(array);
					//js_processType(array, propertyType);

					if (propertyType == JSON_OBJ){
						json_t const *childObj = json_getChild(array);
						
						for (childObj = json_getChild(array); childObj != NULL; childObj = json_getSibling(childObj)){
							jsonType_t propertyType = json_getType(childObj);
							//js_processType(childObj, propertyType);
							
							if (propertyType == JSON_INTEGER){
								char const *name = json_getName(childObj);
								if (!strcmp("chapter", name)){
									int32_t value = (int32_t)json_getInteger(childObj);
									if (value == chapterNo){
										//printf("found chapter %i\n", (int)value);
										hJs->chapter = hJs->json;
										hJs->json = array;
										return hJs->json;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	return NULL;
}

json_t const *js_openVerse (js_t *hJs, const uint32_t verseNo)
{
	//printf("@@@ js_openVerse %i\n", verseNo);
	
	json_t const *child;
	
	for (child = json_getChild(hJs->json); child != NULL; child = json_getSibling(child)){
		jsonType_t propertyType = json_getType(child);
		//js_processType(child, propertyType);
		
		if (propertyType == JSON_ARRAY){
			char const *name = json_getName(child);
			if (!strcmp("verses", name)){
				//printf("found verses \n");
				
				json_t const *array;
				for (array = json_getChild(child); array != NULL; array = json_getSibling(array)){
					jsonType_t propertyType = json_getType(array);
					//js_processType(array, propertyType);

					if (propertyType == JSON_OBJ){
						json_t const *childObj = json_getChild(array);
						
						for (childObj = json_getChild(array); childObj != NULL; childObj = json_getSibling(childObj)){
							jsonType_t propertyType = json_getType(childObj);
							//js_processType(childObj, propertyType);
							
							if (propertyType == JSON_INTEGER){
								char const *name = json_getName(childObj);
								if (!strcmp("verse", name)){
									int32_t value = (int32_t)json_getInteger(childObj);
									if (value == verseNo){
										//printf("found verse %i\n", (int)value);
										hJs->verse = hJs->json;
										hJs->json = array;
										return hJs->json;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return NULL;
}

void js_versePrint (js_t *hJs)
{
	json_t const *child;
	for (child = json_getChild(hJs->json); child != NULL; child = json_getSibling(child)){
		jsonType_t propertyType = json_getType(child);
		//js_processType(child, propertyType);
		
		if (propertyType == JSON_TEXT){
			char const *name = json_getName(child);
			if (!strcmp("text", name)){
				char const *value = json_getValue(child);
				if (value) printf("'%s'\n", value);
			}
		}
	}
}

uint32_t js_verseLength (js_t *hJs)
{
	json_t const *child;
	for (child = json_getChild(hJs->json); child != NULL; child = json_getSibling(child)){
		jsonType_t propertyType = json_getType(child);
		//js_processType(child, propertyType);
		
		if (propertyType == JSON_TEXT){
			char const *name = json_getName(child);
			if (!strcmp("text", name)){
				char const *value = json_getValue(child);
				if (value) return strlen(value)+1;
			}
		}
	}
	return 0;
}

#if 0
char *js_verseDup (js_t *hJs)
{
	json_t const *child;
	for (child = json_getChild(hJs->json); child != NULL; child = json_getSibling(child)){
		jsonType_t propertyType = json_getType(child);
		//js_processType(child, propertyType);
		
		if (propertyType == JSON_TEXT){
			char const *name = json_getName(child);
			if (!strcmp("text", name)){
				char const *value = json_getValue(child);
				if (value) return strdup(value);
			}
		}
	}
	return NULL;
}
#endif

char const *js_verseGet (js_t *hJs)
{
	json_t const *child;
	for (child = json_getChild(hJs->json); child != NULL; child = json_getSibling(child)){
		jsonType_t propertyType = json_getType(child);
		//js_processType(child, propertyType);
		
		if (propertyType == JSON_TEXT){
			char const *name = json_getName(child);
			if (!strcmp("text", name)){
				char const *value = json_getValue(child);
				return value;
			}
		}
	}
	return NULL;
}

void js_verseCopy (js_t *hJs, char *buffer, uint32_t bufferLen)
{
	json_t const *child;
	for (child = json_getChild(hJs->json); child != NULL; child = json_getSibling(child)){
		jsonType_t propertyType = json_getType(child);
		//js_processType(child, propertyType);
		
		if (propertyType == JSON_TEXT){
			char const *name = json_getName(child);
			if (!strcmp("text", name)){
				char const *value = json_getValue(child);
				if (value){
					strncpy(buffer, value, bufferLen);
					buffer[bufferLen-1] = 0;
				}
			}
		}
	}
}

void js_closeChapter (js_t *hJs)
{
	hJs->json = hJs->chapter;
}

void js_closeVerse (js_t *hJs)
{
	hJs->json = hJs->verse;
}

void js_closeBook (js_t *hJs)
{
	hJs->json = hJs->book;
}

#if 0

int main (int argc, char *argv[])
{

	//js_t *hJs = js_importJsonPath("old-testament.json");
	//js_t *hJs = js_importJsonPath("book-of-mormon.json");
	js_t *hJs = js_importJsonBuffer(jsonBook, strlen(jsonBook));
	
	//js_dump(json_getChild(hJs->json), hJs);
	
	
	if (js_openVolumes(hJs, "The Book of Mormon")){
		char *title = js_volumeTitle(hJs);
		if (title){
			printf("Volume title: %s\n\n", title);
			free(title);
		}
		
		if (js_openBook(hJs, "Nephi 2")){
			char *title = js_bookTitle(hJs);
			if (title){
				printf("Book title: %s\n\n", title);
				free(title);
			}

			char *heading = js_bookHeading(hJs);
			if (heading){
				printf("Heading: %s\n\n", heading);
				free(heading);
			}
			
			if (js_openChapter(hJs, 1)){
				if (js_openVerse(hJs, 10)){
					js_versePrint(hJs);
					js_closeVerse(hJs);
				}
					
				if (js_openVerse(hJs, 3)){
					char *verseDup = js_verseDup(hJs);
					if (verseDup){
						printf("verseDup: %s\n\n", verseDup);
						free(verseDup);
					}
					js_closeVerse(hJs);
				}
				js_closeChapter(hJs);
			}

			if (js_openChapter(hJs, 3)){
				if (js_openVerse(hJs, 10)){
					js_versePrint(hJs);
					js_closeVerse(hJs);
				}
					
				if (js_openVerse(hJs, 3)){
					char *verseDup = js_verseDup(hJs);
					if (verseDup){
						printf("verseDup: %s\n\n", verseDup);
						free(verseDup);
					}
					js_closeVerse(hJs);
				}
				js_closeChapter(hJs);
			}
			js_closeBook(hJs);
		}
		
		//jsonStaticSpool_t *spool = (jsonStaticSpool_t*)hJs->spool;
		//printf("%i %i\n", spool->totalMemPools, spool->totalAlloc);
		
		js_free(hJs);
	}

	return EXIT_SUCCESS;
}

#endif

