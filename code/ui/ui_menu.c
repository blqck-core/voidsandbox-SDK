// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/js_local.h"

void MenuDraw(void) {
	vec4_t color = {0.85, 0.9, 1.0, 1};
	qboolean JSWork = JS_MenuCheck();
	int y, i;
	
	if(!JSWork){
	    ST_DrawString(320, 0, "JavaScript Error: UI not initialized!", UI_DROPSHADOW|UI_CENTER, color, 1.50);
    	y = 460 - BASEFONT_HEIGHT;
    	for(i = console.linescount; i > console.linescount - 32; i--) {
    		if(i <= 0) break;
    		ST_DrawString(15 - cgui.wideoffset, y, console.lines[i], UI_DROPSHADOW, color_white, 0.90);
    		y -= BASEFONT_HEIGHT;
    	}
    	return;
	}

	if(strlen(cvarString("com_errorMessage"))) ST_DrawString(0 - cgui.wideoffset, 0, cvarString("com_errorMessage"), UI_DROPSHADOW, color, 1.00);
	
	JS_MenuDraw();
	Menu_ElementsDraw();
}

void UI_Menu(void) {
    if(trap_Key_GetCatcher() & KEYCATCH_UI) return;
	memset(&uis.items, 0, sizeof(menuelement_s)*MAX_MENUITEMS);
    JS_MenuInit();
	trap_Key_SetCatcher(KEYCATCH_UI);
}

void UI_HotReloadJS(void) {
    trap_Cmd(EXEC_NOW, "js.restart \n");
	memset(&uis.items, 0, sizeof(menuelement_s)*MAX_MENUITEMS);
    JS_MenuInit();
	trap_Key_SetCatcher(KEYCATCH_UI);
}
