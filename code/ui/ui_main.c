// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/js_local.h"

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
int vmMain(int command, int arg0, int arg1, int arg2) {
	switch(command) {
	case UI_INIT: UI_Init(); return 0;
	case UI_SHUTDOWN: UI_Shutdown(); return 0;
	case UI_KEY_EVENT: UI_KeyEvent(arg0, arg1); return 0;
	case UI_MOUSE_EVENT: UI_MouseEvent(arg0, arg1); return 0;
	case UI_REFRESH: UI_Refresh(arg0); return 0;
	case UI_IS_FULLSCREEN: return ((trap_Key_GetCatcher() & KEYCATCH_UI) && !uis.onmap);
	case UI_SET_ACTIVE_MENU: UI_SetActiveMenu(arg0); return 0;
	case UI_CONSOLE_COMMAND: return UI_ConsoleCommand(arg0);
	case UI_DRAW_CONNECT_SCREEN: UI_DrawConnectScreen(arg0); return 0;
	case GETVMCONTEXT: VMContext(&vmargs, &vmresult); return 0;
	case VMCALL: VMCall(arg0); return 0;
	//default: UI_Error("ui.qvm: unknown command %i", command); break;
	}
	return -1;
}

const char *gametype_items[GT_MAX_GAME_TYPE + 1] = {"Sandbox", "Free For All", "Team Deathmatch", 0};

console_t console;
int consoleLines = 0;

uimenu_t ui;

int mod_gravity = 800;

void UI_CreateCvars(void) { trap_Cmd(EXEC_APPEND, "exec uiautoexec.cfg\n"); }
