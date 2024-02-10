
/*

*/


#include "libBoM/bom.h"
#include "libBoM/book-of-mormon.json.h"
#include "ui.h"


#define DWIDTH					TFT_WIDTH
#define DHEIGHT					TFT_HEIGHT


typedef struct {
	char const *name;
	uint8_t chapters;
	uint8_t verses[80];
}books_t;

static books_t allbooks[] = {
	{"Nephi 1"},
	{"Nephi 2"},
	{"Jacob"},
	{"Enos"},
	{"Jarom"},
	{"Omni"},
	{"Words of Mormon"},
	{"Mosiah"},
	{"Alma"},
	{"Helaman"},
	{"Nephi 3"},
	{"Nephi 4"},
	{"Mormon"},
	{"Ether"},
	{"Moroni"}
};

static const char *fontfiles[FONT_TOTAL] = {
	UFDIR"10x20.uf",
	UFDIR"consola24.uf",
	UFDIR"teutonweiss-bold30.uf",
	UFDIR"76london38.uf",
	UFDIR"76london50.uf"
};

static _ufont_t fonts[FONT_TOTAL];
static _ufont_surface_t *surface;
static int32_t sceneUpdateRequested;
static uint32_t uiPage = UI_PAGE_STARTUP;
static const ui_page_t *uiPages;

static dial_t dial1;
static dial_t dial2;
static dial_t dial3;
static timerSS_t timerSS;	// to resignal a render update if for whatever reason current failed





static _ufont_t *getFont (const int fontIdx)
{
	return &fonts[fontIdx];
}

static void ufont_init ()
{
	// create a 1BPP work surface. this is the initial work surface
	//surface = fontCreateSurface(DWIDTH, DHEIGHT, COLOUR_24TO16(0xFFBF33), NULL);
	surface = fontCreateSurface(DWIDTH, DHEIGHT, COLOUR_24TO16(0x111111 * 5), NULL);
	// This will fail if memory is short, SDCard is unreadable
	// but more probable, requested font file (.uf) is absent
	for (int i = 0; i < (int)sizeof(fontfiles) / (int)sizeof(*fontfiles); i++){
		_ufont_t *font = getFont(i);
		if (fontOpen(font, fontfiles[i])){
			fontSetDisplayBuffer(font, tft_getBuffer(), DWIDTH, DHEIGHT);
			fontSetRenderSurface(font, surface);
			
			//fontSetGlyphPadding(font, fontGetGlyphPadding(font)+1);
		}else{
			printf("font open failed for, %i,  #%s#", i, fontfiles[i]);
		}
	}
}

void enc1Update (const int value)
{
	if (!(value&0x03)){
		if (value != dial1.pos){
			dial1.posNew = dial1.pos - value;
			dial1.pos = value;
		}
	}
}

void enc2Update (const int value)
{
	if (!(value&0x03)){
		if (value != dial2.pos){
			dial2.posNew = dial2.pos - value;
			dial2.pos = value;
		}
	}
}

void enc3Update (const int value)
{
	if (!(value&0x03)){
		if (value != dial3.pos){
			dial3.posNew = dial3.pos - value;
			dial3.pos = value;
		}
	}
}

static void enc1Reset ()
{
	dial1.posNew = 0;
}

static void enc2Reset ()
{
	dial2.posNew = 0;
}

static void enc3Reset ()
{
	dial3.posNew = 0;
}

void enc1SwCB ()
{
	static int lastPressTime;
	
	int currentPressTime = millis();
	if (currentPressTime - lastPressTime > 400){
		lastPressTime = currentPressTime;
		dial1.swChange = 1;
	}
}

void enc2SwCB ()
{
	static int lastPressTime;
	
	int currentPressTime = millis();
	if (currentPressTime - lastPressTime > 400){
		lastPressTime = currentPressTime;
		dial2.swChange = 1;
	}
}

void enc3SwCB ()
{
	static int lastPressTime;
	
	int currentPressTime = millis();
	if (currentPressTime - lastPressTime > 400){
		lastPressTime = currentPressTime;
		dial3.swChange = 1;
	}
}

void dials_init ()
{
	dial1.pos = -1;
	dial1.posNew = 0;
	dial1.swChange = 0;
	dial1.enc = new Encoder(ENCODER1_PIN_CLK, ENCODER1_PIN_DT, enc1Update);
	
	dial2.pos = -1;
	dial2.posNew = 0;
	dial2.swChange = 0;
	dial2.enc = new Encoder(ENCODER2_PIN_CLK, ENCODER2_PIN_DT, enc2Update);
	
	dial3.pos = -1;
	dial3.posNew = 0;
	dial3.swChange = 0;
	dial3.enc = new Encoder(ENCODER3_PIN_CLK, ENCODER3_PIN_DT, enc3Update);
}

void pins_init ()
{
	pinMode(LED_BUILTIN, OUTPUT);
	
	pinMode(ENCODER1_PIN_SW, INPUT_PULLUP);
	attachInterrupt(ENCODER1_PIN_SW, enc1SwCB, FALLING);

	pinMode(ENCODER2_PIN_SW, INPUT_PULLUP);
	attachInterrupt(ENCODER2_PIN_SW, enc2SwCB, FALLING);
	
	pinMode(ENCODER3_PIN_SW, INPUT_PULLUP);
	attachInterrupt(ENCODER3_PIN_SW, enc3SwCB, FALLING);
}

int drawMain (js_t *hJs, const int book, const int chapter, const int verse)
{
	int success = 0;
	char buffer[32] = {0};
	int width = 0;
	int height = 0;
	int x = 0, y = 0;

	char const *title = js_volumeTitle(hJs);
	if (title){
		_ufont_t *font = getFont(FONT50);
		fontGetMetrics(font, (uint8_t*)title, &width, &height);
		x = (abs(TFT_WIDTH - width)/2) - 2;
		y = 2;
		fontPrint(font, &x, &y, (uint8_t*)title);
	}

	title = js_bookTitle(hJs);
	if (title){
		_ufont_t *font = getFont(FONT38);
		fontGetMetrics(font, (uint8_t*)title, &width, &height);
		x = (abs(TFT_WIDTH - width)/2) - 2;
		y = height + 32;
		fontPrint(font, &x, &y, (uint8_t*)title);
	}
#if 0
	char *heading = js_bookHeading(hJs);
	if (heading){
		//fontGetMetrics(&font30, (uint8_t*)heading, &width, &height);
		x = 4;
		y += height;

		_ufont_t *font = getFont(FONT24);
		fontPrint(font, &x, &y, (uint8_t*)heading);
		free(heading);
	}
#endif

	_ufont_t *font = getFont(FONT20);
	sprintf(buffer, "Chapter: %i / %i", chapter, allbooks[book].chapters);
	x = 12;
	y += height - 20;
	fontPrint(font, &x, &y, (uint8_t*)buffer);

	sprintf(buffer, "Verse: %i / %i", verse, allbooks[book].verses[chapter]);
	x = 12;
	y += 24;
	fontPrint(font, &x, &y, (uint8_t*)buffer);

	if (js_openChapter(hJs, chapter)){
		if (js_openVerse(hJs, verse)){
			char const *verse = js_verseGet(hJs);
			if (verse){
				x = 2;
				y += height;
				success = fontPrint8(getFont(FONT24), &x, &y, (uint8_t*)verse);
			}
			js_closeVerse(hJs);
		}
		js_closeChapter(hJs);
	}
	return success;
}

void sceneStartupDraw (void *opaque)
{
	ui_opaque_info *info = (ui_opaque_info*)opaque;

	drawMain(info->hJs, info->book, info->chapter, info->verse);
	fontApplySurface(getFont(FONT24), 0, 0);
}

static void sceneUpdate ()
{
	sceneUpdateRequested = 1;
}

static void timerSSUpdate_callback ()
{
	//printf("timerSSUpdate_callback\n");
	sceneUpdate();
}

static void timerSSUpdate_start (const uint32_t periodMs)
{
	timerSS.period = periodMs;
	timerSS.func = timerSSUpdate_callback;
	timerSS.time0 = millis();
	timerSS.state = 1;
}

static void timerSSUpdate_end ()
{
	timerSS.state = 0;
}

static void timerSSUpdate_fire ()
{
	timerSSUpdate_end();
	timerSS.func();
}

static void timerSSUpdate_task ()
{
	if (timerSS.state){
		if (millis() - timerSS.time0 > timerSS.period)
			timerSSUpdate_fire();
	}
}

void sceneStartupEnter (void *opaque)
{
	ui_opaque_info *info = (ui_opaque_info*)opaque;

	if (!info->hJs){
		int blen = strlen(json_Book)+1;
		char *source = (char*)extmem_calloc(1, blen+(blen&0x7));
		memcpy(source, json_Book, blen);

		info->hJs = js_importJsonBuffer(source, blen);
		info->flagSw1Ctrl = 1;
		info->book = 0;
		info->chapter = 1;
		info->verse = 1;
	}

	if (info->hJs){
		if (js_openVolumes(info->hJs, "The Book of Mormon")){
			const int totalBooks = sizeof(allbooks) / sizeof(*allbooks);
			for (int b = 0; b < totalBooks; b++){
				if (js_openBook(info->hJs, allbooks[b].name)){
					for (int c = 1; c < 80; c++){
						if (js_openChapter(info->hJs, c)){
							allbooks[b].chapters = c;
							
							for (int v = 1; v < 80; v++){
								if (js_openVerse(info->hJs, v)){
									allbooks[b].verses[c] = v;
									js_closeVerse(info->hJs);
								}
							}
							js_closeChapter(info->hJs);
						}
					}
					js_closeBook(info->hJs);
				}
			}
			js_openBook(info->hJs, allbooks[info->book].name);
		}
	}
	
	const uint32_t wrapFlags = BFONT_RENDER_WORDWRAP|BFONT_RENDER_NEWLINE|BFONT_RENDER_NODEFAULT|BFONT_RENDER_ADVANCE_X|BFONT_RENDER_ADVANCE_Y;
	fontSetRenderFlags(getFont(FONT20), wrapFlags);
	fontSetRenderFlags(getFont(FONT24), wrapFlags);
	//fontSetLineSpace(getFont(FONT24), fontGetLineSpace(getFont(FONT24))+1);
}

void sceneStartupExit (void *opaque)
{
	//ui_opaque_info *info = (ui_opaque_info*)opaque;
	
	//js_closeBook(info->hJs);
	//fontCleanCache(getFont(FONT24));
}

static void uiSetView (const int view)
{
	uiPage = view;
}

const int uiGetView ()
{
	return uiPage;
}

static void uiViewEnter (void *opaque)
{
	const int viewId = uiGetView();
	uiPages[viewId].func.enter(opaque);
}

static void uiViewRender (void *opaque)
{
	const int viewId = uiGetView();
	uiPages[viewId].func.draw(opaque);
}

static void uiViewExit (void *opaque)
{
	const int viewId = uiGetView();
	uiPages[viewId].func.exit(opaque);
}

void *uiGetOpaque (const int pageId)
{
	for (int i = 0; i < UI_PAGE_TOTAL; i++){
		if (uiPages[i].id == pageId)
			return uiPages[i].opaque;
	}
	return NULL;
}

static void *uiSetViewNext ()
{
	int view = uiGetView();
	
	if (++view >= UI_PAGE_TOTAL)
		view = 0;
		
	uiSetView(view);
	return uiGetOpaque(view);
}

static void sceneRender (void *opaque)
{
	//set_arm_clock(600*1000*1000);
	//delay(1);
	
	fontCleanSurface(NULL, surface);
	tft_clearBuffer(COLOUR_CREAM);
	uiViewRender(opaque);
	tft_update();
	
	//delay(1);
	//set_arm_clock(84*1000*1000);
}

static int uiDispatchMessage (const int eventId, uint32_t data1Uint32, int32_t data2Int32, float data3Flt)
{
	return uiEventCB(eventId, uiGetOpaque(uiGetView()), data1Uint32, data2Int32, data3Flt);
}

int startupEnc1Change (void *opaque, const int direction)
{
	ui_opaque_info *info = (ui_opaque_info*)opaque;
		
	if (direction < 0){
		if (info->book > 0)
			info->book--;
	}else{
		const int totalBooks = sizeof(allbooks) / sizeof(*allbooks);
		if (++info->book > totalBooks-1)
			info->book = totalBooks-1;
	}	

	js_closeBook(info->hJs);
	js_openBook(info->hJs, allbooks[info->book].name);
	info->chapter = 1;
	info->verse = 1;

	return 1;
}

int startupEnc2Change (void *opaque, const int direction)
{
	ui_opaque_info *info = (ui_opaque_info*)opaque;
	
	if (direction < 0){
		if (info->chapter > 1)
			info->chapter--;
	}else{
		int chapter = info->chapter + 1;
		if (chapter <= allbooks[info->book].chapters)
			info->chapter = chapter;
	}
	return 1;
}

int startupEnc3Change (void *opaque, const int direction)
{
	ui_opaque_info *info = (ui_opaque_info*)opaque;
	
	if (direction < 0){
		if (info->verse > 1)
			info->verse--;
	}else{
		int verse = info->verse + 1;
		if (verse <= allbooks[info->book].verses[info->chapter])
			info->verse = verse;
	}
	return 1;
}

void startupSw1Change (void *opaque, const int direction)
{
	ui_opaque_info *info = (ui_opaque_info*)uiGetOpaque(UI_PAGE_STARTUP);
		

	if (++info->flagSw1Ctrl > 3)
		info->flagSw1Ctrl = 1;
}

int uiEventCB (const int eventId, void *opaque, uint32_t data1uint32, int32_t data2Int32, float data3Flt)
{
	switch (eventId){
	  case UI_EVENT_ROTARY:
		if (uiGetView() == UI_PAGE_STARTUP){
			//ui_opaque_info *info = (ui_opaque_info*)opaque;
			if (data2Int32 == IN_ROTARY_1)
				return startupEnc1Change(opaque, data3Flt);
			else if (data2Int32 == IN_ROTARY_2)
				return startupEnc2Change(opaque, data3Flt);
			else if (data2Int32 == IN_ROTARY_3)
				return startupEnc3Change(opaque, data3Flt);
		}
		break;
	  case UI_EVENT_BUTTON:
	    if (data2Int32 == IN_SWITCH_1){
			/*uiDispatchMessage(UI_EVENT_PAGE_EXT, 0, 0, 0.0f);
			uiDispatchMessage(UI_EVENT_PAGE_NXT, 0, 0, 0.0f);
			uiDispatchMessage(UI_EVENT_PAGE_ENT, 0, 0, 0.0f);*/
			//startupSw1Change(opaque, data3Flt);
	    	return 1;
	    }
		break;
	  case UI_EVENT_PAGE_NXT:
	    uiSetViewNext();
	    break;
	  case UI_EVENT_PAGE_ENT:
	  	uiViewEnter(opaque);
		break;
	  case UI_EVENT_PAGE_DRW:
	  	sceneRender(opaque);
		break;
	  case UI_EVENT_PAGE_EXT:
	  	uiViewExit(opaque);
		break;
	  case UI_EVENT_TICK:
	  	//usb_task();
		timerSSUpdate_task();
		break;
	};
	return 0;
}

static void ui_init ()
{
	static ui_opaque_t opaque;
	static const ui_page_t _uiPages[UI_PAGE_TOTAL] = {
		{UI_PAGE_INFO,   &opaque.info,   {sceneStartupEnter,sceneStartupDraw,sceneStartupExit}}
	};

	uiPages = _uiPages;
	uiSetView(UI_PAGE_STARTUP);
}

void setup ()
{
	//Serial.begin(2000000);
	//while (!Serial); // wait for Arduino Serial Monitor
	
	//set_arm_clock(600*1000*1000);
	tft_init();
	sd_init();
	ufont_init();
	pins_init();
	dials_init();
	ui_init();
	uiDispatchMessage(UI_EVENT_PAGE_ENT, 0, 0, 0.0f);
	timerSSUpdate_start(5);
	//sceneUpdate();
}

void loop ()
{

	// Rotary 1
	if (dial1.posNew){
		if (uiDispatchMessage(UI_EVENT_ROTARY, 0, IN_ROTARY_1, dial1.posNew))
			sceneUpdate();
		enc1Reset();
	}

	if (dial1.swChange){
		if (uiDispatchMessage(UI_EVENT_BUTTON, 0, IN_SWITCH_1, dial1.swChange))
			sceneUpdate();
		dial1.swChange = 0;
	}

	// Rotary 2
	if (dial2.posNew){
		if (uiDispatchMessage(UI_EVENT_ROTARY, 0, IN_ROTARY_2, dial2.posNew))
			sceneUpdate();
		enc2Reset();
	}

	if (dial2.swChange){
		if (uiDispatchMessage(UI_EVENT_BUTTON, 0, IN_SWITCH_2, dial2.swChange))
			sceneUpdate();
		dial2.swChange = 0;
	}
	
	// Rotary 3
	if (dial3.posNew){
		if (uiDispatchMessage(UI_EVENT_ROTARY, 0, IN_ROTARY_3, dial3.posNew))
			sceneUpdate();
		enc3Reset();
	}

	if (dial3.swChange){
		if (uiDispatchMessage(UI_EVENT_BUTTON, 0, IN_SWITCH_3, dial3.swChange))
			sceneUpdate();
		dial3.swChange = 0;
	}
	
	if (uiDispatchMessage(UI_EVENT_TICK, 0, 0, 0.0f))
		sceneUpdate();

	if (sceneUpdateRequested){
		sceneUpdateRequested = 0;
		uiDispatchMessage(UI_EVENT_PAGE_DRW, 0, 0, 0.0f);
	}

#if 1
	delay(1);
#else
	dumpStats();
	delay(30);
#endif
}

