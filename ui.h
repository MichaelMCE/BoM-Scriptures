
#ifndef _UI_H_
#define _UI_H_

#include <Arduino.h>
#include "encoder/encoder.h"
#include "uFont/common.h"



#define IN_ROTARY_1				1001
#define IN_ROTARY_2				1002
#define IN_ROTARY_3				1003

#define IN_SWITCH_1				1011
#define IN_SWITCH_2				1012
#define IN_SWITCH_3				1013

#define ENCODER1_PIN_CLK		30
#define ENCODER1_PIN_DT			31
#define ENCODER1_PIN_SW			32

#define ENCODER2_PIN_CLK		2
#define ENCODER2_PIN_DT			3
#define ENCODER2_PIN_SW			4

#define ENCODER3_PIN_CLK		5
#define ENCODER3_PIN_DT			6
#define ENCODER3_PIN_SW			7

#define ENCODER_SW_DEBOUNCE		350


enum _uiEvents {
	UI_EVENT_ROTARY = 1,		// dataInt32=which, dataFlt=direction
	UI_EVENT_BUTTON,			// dataInt32=which.	includes internal rotary switch
	UI_EVENT_PAGE_ENT,			// 
	UI_EVENT_PAGE_DRW,			// 
	UI_EVENT_PAGE_EXT,			// 
	UI_EVENT_PAGE_NXT,		
	UI_EVENT_TICK			
};

enum _uiPages {
	UI_PAGE_INFO,
	UI_PAGE_TOTAL,
	UI_PAGE_STARTUP	= UI_PAGE_INFO
};

enum _fonts {
	FONT20,
	FONT24,
	FONT38,
	FONT50,
	FONT_TOTAL
};


typedef struct _uiview {
	void (*enter) (void *opaque);
	void (*draw)  (void *opaque);
	void (*exit)  (void *opaque);
}ui_view_t;

typedef struct _page {
	int id;
	void *opaque;
	ui_view_t func;
}ui_page_t;


typedef struct {
	js_t *hJs;
	int chapter;
	int verse;
	int book;
	
	int flagSw1Ctrl;
	char const *bookHeading;
}ui_opaque_info;


typedef struct {
	ui_opaque_info info;
	// ui_opaque_info anotherPage;
}ui_opaque_t;


const int uiGetView ();
void *uiGetOpaque (const int pageId);
int uiEventCB (const int eventId, void *opaque, uint32_t data1uint32, int32_t data2Int32, float data3Flt);


typedef struct {
	Encoder *enc;
	volatile int posNew;
	volatile int swChange;
	int pos;
}dial_t;

typedef struct {
	uint32_t time0;
	int32_t state;
	uint32_t period;
	void (*func) ();
}timerSS_t;




typedef struct {
	int16_t offset = 0;
	int16_t width;
	int16_t preset = -1;
	
	int8_t pause = 1;
	int8_t dir = 1;
}ui_scroll_t;


#ifndef set_arm_clock
extern "C" uint32_t set_arm_clock (uint32_t frequency);
#endif




#endif

