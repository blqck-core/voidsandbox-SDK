// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

qboolean passwordNeeded = qtrue;
menuelement_s passwordField;

static connstate_t lastConnState;
static char lastLoadingText[MAX_INFO_VALUE];

/*
========================
UI_DrawConnectScreen

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
========================
*/
#define LOADING_LINES 8
void UI_DrawConnectScreen(qboolean overlay) {
	char *s;
	uiClientState_t cstate;
	char info[MAX_INFO_VALUE];
	qhandle_t black;
	qhandle_t logo;
	qhandle_t loading;
	int i, y;

	consoleSync(&console, console.linescount);

	// see what information we should display
	trap_GetClientState(&cstate);

	info[0] = '\0';
	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

	Menu_Cache();

	black = trap_R_RegisterShaderNoMip("gfx/colors/black");
	logo = trap_R_RegisterShaderNoMip("menu/logo");
	loading = trap_R_RegisterShaderNoMip("menu/assets/loading");

	// print any server info (server full, bad version, etc)
	if(cstate.connState < CA_CONNECTED) {
		ST_DrawString(2 - cgui.wideoffset, 2, cstate.messageString, UI_CENTER | UI_DROPSHADOW, color_white, 1.00);
	}
	if(lastConnState > cstate.connState) {
		lastLoadingText[0] = '\0';
	}
	lastConnState = cstate.connState;

	y = 460 - BASEFONT_HEIGHT;
	for(i = console.linescount; i > console.linescount - LOADING_LINES; i--) {
		if(i <= 0) break;
		ST_DrawString(15 - cgui.wideoffset, y, console.lines[i], UI_DROPSHADOW, color_white, 0.90);
		y -= BASEFONT_HEIGHT;
	}

	switch(cstate.connState) {
	case CA_CONNECTING: s = va("Awaiting challenge...%i", cstate.connectPacketCount); break;
	case CA_CHALLENGING: s = va("Awaiting connection...%i", cstate.connectPacketCount); break;
	case CA_CONNECTED: s = "Awaiting gamestate..."; break;
	case CA_LOADING: return;
	case CA_PRIMED: return;
	default: return;
	}

	UI_DrawHandlePic(0 - (cgui.wideoffset + 1), 0, SCREEN_WIDTH + (cgui.wideoffset * 2) + 2, SCREEN_HEIGHT * 3, black);
	UI_DrawHandlePic(320 - 50, 240 - 75, 100, 100, logo);
	UI_DrawHandlePic(320 - 24, 320 - 48, 48, 48, loading);

	ST_DrawString(2 - cgui.wideoffset, 2, s, UI_DROPSHADOW, color_white, 1.00);
}
