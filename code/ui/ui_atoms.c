// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

uiStatic_t uis;

void QDECL Com_Error(int level, const char *error, ...) {
	va_list argptr;
	char text[1024];
	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(va("%s", text));
}

void QDECL Com_Printf(const char *msg, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(va("%s", text));
}

void UI_OnMapStatus(void) {
	char svinfo[MAX_INFO_STRING];

	trap_GetConfigString(CS_SERVERINFO, svinfo, MAX_INFO_STRING);
	if(strlen(svinfo) <= 0) {
		uis.onmap = qfalse;
	} else {
		uis.onmap = qtrue;
	}
}

void UI_PushMenu(menuframework_s *menu) {
	int i;
	menucommon_s *item;

	// default cursor position
	menu->cursor = 0;
	menu->cursor_prev = 0;

	trap_Key_SetCatcher(KEYCATCH_UI);

	// force first available item to have focus
	for(i = 0; i < menu->nitems; i++) {
		item = (menucommon_s *)menu->items[i];
		if(!(item->flags & (QMF_GRAYED | QMF_INACTIVE))) {
			menu->cursor_prev = -1;
			Menu_SetCursor(menu, i);
			break;
		}
	}

	uis.firstdraw = qtrue;
}

void UI_PopMenu(void) {
	trap_S_StartLocalSound(menu_out_sound, CHAN_LOCAL_SOUND);
	UI_ForceMenuOff();
}

void UI_ForceMenuOff(void) {
	trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
	trap_Key_ClearStates();
}

void UI_SetActiveMenu(uiMenuCommand_t menu) {
    ST_UpdateCGUI();
    UI_OnMapStatus();
	Menu_Cache();

	switch(menu) {
	case UIMENU_NONE: UI_ForceMenuOff(); return;
	case UIMENU_MAIN: UI_Menu(); return;
	case UIMENU_INGAME: UI_Menu(); return;
	default: Com_Printf("UI_SetActiveMenu: bad enum %d\n", menu); break;
	}
}

void UI_KeyEvent(int key, int down) {
	sfxHandle_t s;

	if(!down) return;

	s = Menu_DefaultKey(key);

	if((s > 0) && (s != menu_null_sound)) trap_S_StartLocalSound(s, CHAN_LOCAL_SOUND);
}

void UI_MouseEvent(int dx, int dy) {
	int i;

	// update mouse screen position
	uis.cursorx += dx;
	if(uis.cursorx < 0 - cgui.wideoffset) uis.cursorx = 0 - cgui.wideoffset;
	else if(uis.cursorx > 640 + cgui.wideoffset) uis.cursorx = 640 + cgui.wideoffset;

	uis.cursory += dy;
	if(uis.cursory < 0) uis.cursory = 0;
	else if(uis.cursory > 480) uis.cursory = 480;

	for(i = 0; i < MAX_MENUITEMS; i++) {
		if(uis.items[i].flags & (QMF_INACTIVE)) continue;
		if(!Menu_CursorOnItem(uis.items[i].id)) continue;
		uis.currentItem = i;
		return;
	}
}

static char *UI_Argv(int arg) {
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}

qboolean UI_ConsoleCommand(int realTime) {
	char *cmd;
	cmd = UI_Argv(0);

	// ensure minimum menu data is available
	Menu_Cache();

	if(Q_stricmp(UI_Argv(0), "ui_spawnmenu") == 0) {
		//UI_SpawnMenu();
		return qtrue;
	}

	if(Q_stricmp(cmd, "menuback") == 0) {
		UI_PopMenu();
		return qtrue;
	}

	return qfalse;
}

void UI_Shutdown(void) {}

void UI_Init(void) {
	// init cvars and gameinfo
	UI_CreateCvars();
	ST_RegisterCvars();

	UI_InitMemory();
	UI_LoadArenas();
	UI_LoadBots();
	
	ST_InitCGUI("default");

	// initialize the menu system
	Menu_Cache();
}

void UI_DrawHandlePic(float x, float y, float w, float h, qhandle_t hShader) {
	float s0;
	float s1;
	float t0;
	float t1;

	if(w < 0) { // flip about vertical
		w = -w;
		s0 = 1;
		s1 = 0;
	} else {
		s0 = 0;
		s1 = 1;
	}

	if(h < 0) { // flip about horizontal
		h = -h;
		t0 = 1;
		t1 = 0;
	} else {
		t0 = 0;
		t1 = 1;
	}

	ST_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, hShader);
}

void UI_DrawModelElement(float x, float y, float w, float h, const char *model, float scale) {
	refdef_t refdef;
	refEntity_t ent;
	vec3_t origin;
	vec3_t angles;

	memset(&refdef, 0, sizeof(refdef));
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear(refdef.viewaxis);

	ST_AdjustFrom640(&x, &y, &w, &h);
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.fov_x = 90;
	refdef.fov_y = 90;

	refdef.time = uis.realtime;

	origin[0] = scale;
	origin[1] = 0;
	origin[2] = 0;

	angles[0] = 0;
	angles[1] = (uis.realtime & 4095) * 360 / 4096.0;
	angles[2] = 0;

	trap_R_ClearScene();

	memset(&ent, 0, sizeof(ent));

	AnglesToAxis(angles, ent.axis);
	ent.hModel = trap_R_RegisterModel(model);
	ent.shaderRGBA[0] = 128;
	ent.shaderRGBA[1] = 128;
	ent.shaderRGBA[2] = 128;
	ent.shaderRGBA[3] = 255;
	VectorCopy(origin, ent.origin);
	VectorCopy(origin, ent.lightingOrigin);
	ent.renderfx = RF_LIGHTING_ORIGIN;
	VectorCopy(ent.origin, ent.oldorigin);
	ent.customSkin = trap_R_RegisterSkin(va("mtr/%s/%i.skin", model, 0));

	trap_R_AddRefEntityToScene(&ent);
	trap_R_RenderScene(&refdef);
}

void UI_UpdateScreen(void) { trap_UpdateScreen(); }

void UI_Refresh(int realtime) {
	int x;
	uis.frametime = realtime - uis.realtime;
	uis.realtime = realtime;

	if(!(trap_Key_GetCatcher() & KEYCATCH_UI)) return;

	ST_UpdateCGUI();
	UI_OnMapStatus();
	consoleSync(&console, console.linescount);

	if(!uis.onmap) trap_R_DrawStretchPic(0.0, 0.0, glconfig.vidWidth, glconfig.vidHeight, 0, 0, 1, 1, uis.menuWallpapers);
	trap_R_DrawStretchPic(0.0, 0.0, glconfig.vidWidth, glconfig.vidHeight, 0, 0, 0.5, 1, trap_R_RegisterShaderNoMip("menu/assets/blacktrans2"));

	MenuDraw();

	if(uis.firstdraw) {
		UI_MouseEvent(0, 0);
		uis.firstdraw = qfalse;
	}

	// draw cursor
	trap_R_SetColor(NULL);
	UI_DrawHandlePic(uis.cursorx - 16, uis.cursory - 16, 32, 32, uis.cursor);

	if(uis.debug) {
		x = 0 - cgui.wideoffset;
		ST_DrawString(x, 0, va("cursor xy: (%d,%d)", uis.cursorx, uis.cursory), UI_LEFT, color_white, 1.00);
		ST_DrawString(x, 10, va("screen: %ix%i", glconfig.vidWidth, glconfig.vidHeight), UI_LEFT, color_white, 1.00);
		ST_DrawString(x, 20, va("map running: %i", uis.onmap), UI_LEFT, color_white, 1.00);
	}
}
