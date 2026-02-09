// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../shared/javascript.h"

static void CG_ScoresDown_f(void) {
	if(cg.scoresRequestTime + 2000 < cg.time) {
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand("score");

		if(!cg.showScores) {
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	} else {
		cg.showScores = qtrue;
	}
}

static void CG_ScoresUp_f(void) {
	if(cg.showScores) cg.showScores = qfalse;
}

static void CG_ReplaceTexture_f(void) {
	char oldtexture[1024];
	char newtexture[1024];

	trap_Argv(1, oldtexture, sizeof(oldtexture));
	trap_Argv(2, newtexture, sizeof(newtexture));

	trap_R_RemapShader(oldtexture, newtexture, "0.005");
}

typedef struct {
	char *cmd;
	void (*function)(void);
} consoleCommand_t;

static consoleCommand_t commands[] = {
    {"+scores", CG_ScoresDown_f},
    {"-scores", CG_ScoresUp_f},
    {"+zoom", CG_ZoomDown_f},
    {"-zoom", CG_ZoomUp_f},
    {"weapnext", CG_NextWeapon_f},
    {"weapprev", CG_PrevWeapon_f},
    {"weapon", CG_Weapon_f},
    {"changetexture", CG_ReplaceTexture_f},
};

qboolean CG_ConsoleCommand(void) {
	const char *cmd;
	int i;

	cmd = CG_Argv(0);

	for(i = 0; i < ARRAY_SIZE(commands); i++) {
		if(!Q_stricmp(cmd, commands[i].cmd)) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}

void CG_InitConsoleCommands(void) {
	int i;

	for(i = 0; i < ARRAY_SIZE(commands); i++) {
		trap_AddCommand(commands[i].cmd);
	}

	// can be used even during intermission
	trap_AddCommand("say");
	trap_AddCommand("say_team");
	trap_AddCommand("tell");
	trap_AddCommand("team");
	trap_AddCommand("score");

	// cheats
	trap_AddCommand("give");
	trap_AddCommand("god");
	trap_AddCommand("tp");
	trap_AddCommand("noclip");

	// interaction
	trap_AddCommand("exitvehicle");
	trap_AddCommand("kill");
	trap_AddCommand("flashlight");
	trap_AddCommand("undo");
	trap_AddCommand("activate");

	// game commands
	trap_AddCommand("follownext");
	trap_AddCommand("followprev");
	trap_AddCommand("freespectator");

	// server commands
	trap_AddCommand("entityList");
	trap_AddCommand("forceTeam");
	trap_AddCommand("addbot");
	trap_AddCommand("savemap");
	trap_AddCommand("deletemap");
	trap_AddCommand("clearmap");
	trap_AddCommand("loadmap");
}
