// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/js_local.h"

void CG_LoadingString(const char *s, float value) {
	Q_StringCopy(cg.infoScreenText, va("Loading... %s", s), sizeof(cg.infoScreenText));
	if(value != -1) cg.infoScreenValue = value;
	trap_UpdateScreen();
}

void CG_LoadingClient(int clientNum) {
	const char *info;
	char personality[MAX_QPATH];

	info = CG_ConfigString(CS_PLAYERS + clientNum);
	Q_StringCopy(personality, Info_ValueForKey(info, "n"), sizeof(personality));
	Q_CleanStr(personality);
	CG_LoadingString(personality, -1);
}

void CG_DrawInformation(void) {
	const char *s;
	const char *info;
	int y;
	qhandle_t levelshot;
	qhandle_t detail;
	qhandle_t fade;
	qhandle_t logo;
	qhandle_t loading;
	char buf[64];
	vec4_t color_white = {1.00f, 1.00f, 1.00f, 1.00f};
	vec4_t color_whiteblack = {0.90f, 0.90f, 0.90f, 1.00f};
	vec4_t color_grey = {0.30f, 0.30f, 0.30f, 1.00f};
	vec4_t color_lightgrey = {0.50f, 0.50f, 0.50f, 1.00f};

	info = CG_ConfigString(CS_SERVERINFO);

	s = Info_ValueForKey(info, "sv_mapname");
	levelshot = trap_R_RegisterShaderNoMip(va("levelshots/%s", s));
	if(!levelshot) levelshot = trap_R_RegisterShaderNoMip("menu/assets/unknownmap");
	detail = trap_R_RegisterShaderNoMip("menu/assets/loadingoverlay");
	fade = trap_R_RegisterShaderNoMip("menu/assets/blacktrans");
	logo = trap_R_RegisterShaderNoMip("menu/logo");
	loading = trap_R_RegisterShaderNoMip("menu/assets/loading");
	trap_R_SetColor(NULL);

	CG_DrawPic(0 - cgui.wideoffset, 0, 640 + (cgui.wideoffset * 2), 480, levelshot);
	CG_DrawPic(0 - cgui.wideoffset, 0, 640 + (cgui.wideoffset * 2), 480, detail);

	ST_DrawRoundedRect(410 + cgui.wideoffset, 445, 220, 30, 1, color_lightgrey);
	CG_DrawProgressBar(415 + cgui.wideoffset, 459, 210, 12, cg.infoScreenValue, 8, color_white, color_grey);
	ST_DrawString(415 + cgui.wideoffset, 449, cg.infoScreenText, UI_LEFT, color_whiteblack, 0.80);

	CG_DrawPic(320 - 50, 240 - 75, 100, 100, logo);
	CG_DrawPic(320 - 24, 320 - 48, 48, 48, loading);

	CG_DrawPic(0 - cgui.wideoffset, 0, 300, 85, fade);
	CG_DrawPic(5 - cgui.wideoffset, 5, 100, 75, levelshot);

	y = 10;
	Q_StringCopy(buf, Info_ValueForKey(info, "sv_hostname"), 64);
	Q_CleanStr(buf);
	ST_DrawString(110 - cgui.wideoffset, y, buf, UI_LEFT, color_white, 1.50);
	y += 25;

	Q_StringCopy(buf, Info_ValueForKey(info, "sv_mapname"), 64);
	Q_CleanStr(buf);
	ST_DrawString(110 - cgui.wideoffset, y, buf, UI_LEFT, color_white, 1.50);
	y += 25;

	switch(cgs.gametype) {
	case GT_SANDBOX: s = "Sandbox"; break;
	case GT_FFA: s = "Free For All"; break;
	case GT_TEAM: s = "Team Deathmatch"; break;
	default: s = "Unknown Gametype"; break;
	}

	ST_DrawString(110 - cgui.wideoffset, y, s, UI_LEFT, color_white, 1.50);
}
